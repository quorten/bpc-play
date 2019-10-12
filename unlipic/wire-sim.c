/* Modular hardware simulator that uses digital (i.e. binary) "wires"
   as the connection between separate hardware modules.  Gate arrays
   are also simulated as "hardware modules."  */

/*

TODO NEW ARCHITECTURE:

* Array of hardware simulators
* Single memory image passed in
* Each simulator has its own internal memory
* There is the shared wire area that connects everything together
* Init, resume, step, suspend, shutdown
* The engine determines which units need to be simulated
  by their magic number specifications in the input.
* All simulator options are configured through that image.
* The simulator supports snapshotting.
* Resume snapshot is automatic via the loading protocol.
* Snapshots can be saved periodically.
* Snapshots can be saved on signals.
* Single snapshot file, multiple snapshot files.

* Matter of fact, the array of simulators... is embarrassingly
  parallel.  It's just that the data synchronization is what makes it
  not so efficient to do in full parallel.  Yes, it's the /wires/!

*/

#include "bool.h"
#include "exparray.h"

EA_TYPE(char);

typedef unsigned char byte;
EA_TYPE(byte);
typedef byte_array HwMem;

enum HwState { HWS_ACTIVE, HWS_SOFT_SUSP, HWS_HARD_SUSP };

typedef struct HwContext_tag HwContext;
struct HwContext_tag {
  /* How many bits are in one addressable wire/memory unit?  This is
     also used to save memory for units less than 8 bits wide.

     1-bit: 8 packed to a byte
     2-bit: 4 packed to a byte
     3-bit: 2 packed to a byte
     4-bit: 2 packed to a byte
     5-bit to 8-bit: 1 packed to a byte
     16-bit: 2 bytes per unit
     32-bit: 4 bytes per unit
     64-bit: 8 bytes per unit
     128-bit: 16 bytes per unit

   */
  unsigned char bits_per_unit;
  /* 0: Digital/binary data
     1: Analog, unsigned integer digital data type
     2: Analog, signed integer digital data type
     3: Analog, fixed point digital data type
     4: Analog, floating point digital data type
   */
  unsigned char analog_mode;
  unsigned wire_base;
  unsigned wire_max;
  unsigned mem_base;
  unsigned mem_max;
  HwMem local_mem;
  /* 0: Active
     1: Soft suspend, simulation stepping is not performed on this
        hardware device, but it is still resumed.
     2: Hard suspend, this hardware device was fully suspended and
        needs to be resumed before use.
   */
  unsigned char active_state;
};

EA_TYPE(HwContext);

typedef bool (*HwHook) (HwContext*);

typedef struct HwClass_tag HwClass;
struct HwClass_tag {
  unsigned magic;
  HwHook resume;
  HwHook step;
  HwHook suspend;
  HwContext_array insts;
};

EA_TYPE(HwClass);

/* Array of available hardware classes.  These must be sorted by magic
   number in ascending order.  */
static HwClass g_hw_avail_d[] = {};
#define G_HW_AVAIL_LEN 0

#define BIT_ARRAY_GET(array, pos) \
  ((array)[(pos)>>3] >> ((pos) & (8 - 1)))
#define BIT_ARRAY_GET_BYTE(array, pos) \
  ((array)[(pos)>>3])

/* If clock is zero, do nothing.  Otherwise, it should be set to 1, or
   else the strange things will happen.  */
#define BIT_ARRAY_SET(array, pos, clock) \
  ((array)[(pos)>>3] |=  ((clock) << ((pos) & (8 - 1))))

#define BIT_ARRAY_CLEAR(array, pos, clock) \
  ((array)[(pos)>>3] &= ~((clock) << ((pos) & (8 - 1))))

struct WireSimCtx_tag {
  unsigned magic;
  unsigned version;

  /* Simulation options */
  /* If true, zero the values in "new_wires" at the start of every
     simulation step.  */
  bool zero_wires : 1;
  /* If true, take a snapshot upon receiving SIGINT.  */
  bool snap_signal : 1;
  /* If true, save multiple snapshots with ascending numeric names.
     Otherwise, overwrite a single snapshot file.  */
  bool snap_multi : 1;
  /* Period, in simulation cycles, upon which to take a snapshot.  If
     zero, disable periodic snapshot saving.  */
  unsigned snap_period;
  /* Filename to use for snapshot(s).  */
  char_array snap_filename;

  /* We switch back and forth between two wire buffers so that we
     don't mix up old values with new values.  That is, we only read
     the from old values and write to new values within a time
     frame.  */
  byte *wires[2];
  unsigned num_wires;
  /* On the other hand, memory is sequential access, so we only have
     one memory buffer.  */
  byte_array mem;
  unsigned long long sim_time = 0;
  HwClass_array hw;
};

static int
hw_magic_cmp_fn (const void *a, const void *b)
{
  const HwClass *ca = (const HwClass*) a;
  const HwClass *cb = (const HwClass*) b;
  return ca->magic - cb->magic;
}

WireSimContext *
wire_sim_read_image (FILE *fp)
{
  /* This is a pretty straightforward operation.  Just allocate the
     data structures at the necessary sizes, read in the data, and
     rebase the pointers.  */
  size_t result;

  /* TODO: First read magic and version.  Fail if magic doesn't match,
     then use the appropriate version loading function, or else
     fail.  */

  /* TODO: Evaluate the option of inserting size fields into the data
     structures, so that there is room for expansion.  Verdict:
     Doesn't sound like a good idea.  This simulator is supposed to be
     *simple* and *complete*, which means that no essential data
     should get ignored.

     Okay, that being said, there is indeed room then for expansion,
     like additional fields specifying hardware acceleration, and
     IOMMU support.  */

  /* READ IMAGE V1 */

  WireSimContext *ctx = (WireSimContext*) xmalloc (sizeof (WireSimContext));
  result = fread (ctx, sizeof (WireSimContext), 1, fp);
  /* if (result != 1) goto fail; */

  fseek (fp, (long) ctx->snap_filename.d, SEEK_SET);
  ctx->snap_filename.d = (char*)
    xmalloc (sizeof (char) * ctx->snap_filename.len);
  result = fread (ctx->snap_filename.d,
		  sizeof (char) * ctx->snap_filename.len, 1, fp);
  /* if (result != 1) goto fail; */
  fseek (fp, (long) ctx->wires[0], SEEK_SET);
  ctx->wires[0] = (byte*) xmalloc (sizeof (byte) * ctx->num_wires);
  result = fread (ctx->wires[0], sizeof (byte) * ctx->num_wires, 1, fp);
  /* if (result != 1) goto fail; */
  fseek (fp, (long) ctx->wires[1], SEEK_SET);
  ctx->wires[1] = (byte*) xmalloc (sizeof (byte) * ctx->num_wires);
  result = fread (ctx->wires[1], sizeof (byte) * ctx->num_wires, 1, fp);
  /* if (result != 1) goto fail; */
  fseek (fp, (long) ctx->mem.d, SEEK_SET);
  ctx->mem.d = (byte*) xmalloc (sizeof (byte) * ctx->mem.len);
  result = fread (ctx->mem.d, sizeof (byte) * ctx->mem.len, 1, fp);
  /* if (result != 1) goto fail; */

  {
    unsigned i;
    fseek (fp, (long) ctx->hw.d, SEEK_SET);
    ctx->hw.d = (HwClass*) xmalloc (sizeof (HWClass) * ctx->hw.len);
    result = fread (ctx->hw.d, sizeof (HWClass) * ctx->hw.len, 1, fp);
    /* if (result != 1) goto fail; */
    for (i = 0; i < ctx->hw.len; i++) {
      /* Now this is where things get a little bit tricky.  The
	 function pointers for the hardware classes will essentially
	 be NULL when we read in the data structures, so now we fill
	 in the appropriate values based off of the magic number.  */
      unsigned j;
      HwClass *cur_hw = &ctx->hw.d[i];
      HwClass *match = bsearch (cur_hw,
				g_hw_avail_d,
				G_HW_AVAIL_LEN,
				sizeof (HwClass),
				hw_magic_cmp_fn);
      if (match == NULL)
	/* goto fail; */;
      cur_hw->resume = match->resume;
      cur_hw->step = match->step;
      cur_hw->suspend = match->suspend;

      fseek (fp, (long) cur_hw->insts.d, SEEK_SET);
      cur_hw->insts.d = (HwContext*)
	xmalloc (sizeof (HwContext) * cur_hw->insts.len);
      result = fread (cur_hw->insts.d,
		      sizeof (HwContext) * cur_hw->insts.len, 1, fp);
      /* if (result != 1) goto fail; */

      for (j = 0; j < cur_hw->insts.len; j++) {
	HwContext *cur_hw_ctx = &cur_hw->insts.d[j];
	fseek (fp, (long) cur_hw_ctx->local_mem.d, SEEK_SET);
	cur_hw_ctx->local_mem.d = (byte*)
	  xmalloc (sizeof (byte) * cur_hw_ctx->local_mem.len);
	result = fread (cur_hw_ctx->local_mem.d,
			sizeof (byte) * cur_hw_ctx->local_mem.len,
			1, fp);
	/* if (result != 1) goto fail; */
      }
    }
  }

  return ctx;
}

bool
wire_sim_write_snapshot (WireSimContext *ctx)
{
  size_t result;

  /* TODO: Rebase pointers to file offsets while writing.  */

  void *save_snap_filename;
  void *save_wires_0;
  void *save_wires_1;
  void *save_mem_d;
  void *save_hw_d;

  size_t snap_filename_ofs;
  size_t wires_0_ofs;
  size_t wires_1_ofs;
  size_t mem_d_ofs;
  size_t hw_d_ofs;

  result = fwrite (ctx, sizeof (WireSimContext), 1, fp);
  /* if (result != 1) goto fail; */

  result = fwrite (ctx->snap_filename.d,
		   sizeof (char) * ctx->snap_filename.len, 1, fp);
  /* if (result != 1) goto fail; */
  result = fwrite (ctx->wires[0], sizeof (byte) * ctx->num_wires, 1, fp);
  /* if (result != 1) goto fail; */
  result = fwrite (ctx->wires[1], sizeof (byte) * ctx->num_wires, 1, fp);
  /* if (result != 1) goto fail; */
  result = fwrite (ctx->mem.d, sizeof (byte) * ctx->mem.len, 1, fp);
  /* if (result != 1) goto fail; */

  {
    unsigned i;
    /* TODO FIXME: This is a sticky situation.  Rebasing pointers in
       expandable arrays.  */
    void *save_insts_d;
    size_t insts_d_ofs;
    result = fwrite (ctx->hw.d, sizeof (HWClass) * ctx->hw.len, 1, fp);
    /* if (result != 1) goto fail; */
    for (i = 0; i < ctx->hw.len; i++) {
      HwHook save_resume = cur_hw->resume;
      HwHook save_step = cur_hw->step;
      HwHook save_suspend = cur_hw->suspend;
      void *save_local_mem_d;
      size_t local_mem_d_ofs;

      cur_hw->resume = NULL;
      cur_hw->step = NULL;
      cur_hw->suspend = NULL;

      result = fwrite (cur_hw->insts.d,
		       sizeof (HwContext) * cur_hw->insts.len, 1, fp);
      /* if (result != 1) goto fail; */

      for (j = 0; j < cur_hw->insts.len; j++) {
	HwContext *cur_hw_ctx = &cur_hw->insts.d[j];
	result = fwrite (cur_hw_ctx->local_mem.d,
			 sizeof (byte) * cur_hw_ctx->local_mem.len,
			 1, fp);
	/* if (result != 1) goto fail; */
      }
    }
  }
  return true;
}

bool
wire_sim_free_image (WireSimContext *ctx)
{
  unsigned i;
  for (i = 0; i < ctx->hw.len; i++) {
    unsigned j;
    HwClass *cur_hw = &ctx->hw.d[i];
    for (j = 0; j < cur_hw->insts.len; j++) {
      HwContext *cur_hw_ctx = &cur_hw->insts.d[j];
      xfree (cur_hw_ctx->local_mem.d);
    }
    xfree (cur_hw->insts.d);
  }
  xfree (ctx->hw.d);
  xfree (ctx->mem.d);
  xfree (ctx->wires[1]);
  xfree (ctx->wires[0]);
  xfree (ctx->snap_filename.d);
  xfree (ctx);
  return true;
}

/* Resume all active or soft-suspend hardware at the beginning of a
   global simulation resume.  */
bool
hw_resume (WireSimContext *ctx)
{
  unsigned i;
  for (i = 0; i < ctx->hw.len; i++) {
    unsigned j;
    HwClass *hw_class = ctx->hw.d[i];
    HwHook hw_resume = hw_class->resume;
    for (j = 0; j < hw_class->insts.len; j++) {
      HwContext *hw_ctx = hw_class->insts.d[j];
      if (hw_ctx->active_state == HWS_ACTIVE ||
	  hw_ctx->active_state == HWS_SOFT_SUSP) {
	bool result = hw_resume (hw_ctx);
	if (!result)
	  return false;
      }
    }
  }
  return true;
}

bool
hw_step (WireSimContext *ctx)
{
  unsigned i;
  for (i = 0; i < ctx->hw.len; i++) {
    unsigned j;
    HwClass *hw_class = ctx->hw.d[i];
    HwHook hw_step = hw_class->step;
    for (j = 0; j < hw_class->insts.len; j++) {
      HwContext *hw_ctx = hw_class->insts.d[j];
      bool result;
      /* Only step-simulate hardware units that are active.  */
      if (hw_ctx->active_state == HWS_ACTIVE) {
	result = hw_step (hw_ctx);
	if (!result)
	  return false;
      }
    }
  }
  return true;
}

/* Suspend all active or soft-suspend hardware at the beginning of a
   global simulation suspend.  */
bool
hw_suspend (WireSimContext *ctx)
{
  bool retval = true;
  unsigned i;
  for (i = 0; i < ctx->hw.len; i++) {
    unsigned j;
    HwClass *hw_class = ctx->hw.d[i];
    HwHook hw_suspend = hw_class->suspend;
    for (j = 0; j < hw_class->insts.len; j++) {
      HwContext *hw_ctx = hw_class->insts.d[j];
      if (hw_ctx->active_state == HWS_ACTIVE ||
	  hw_ctx->active_state == HWS_SOFT_SUSP) {
	bool result = hw_suspend (hw_ctx);
	if (!result)
	  retval = false;
      }
    }
  }
  return retval;
}

int
main (int argc, char *argv[])
{
  int retval = 0;
  FILE *fp;
  WireSimContext *ctx = NULL;

  if (argc != 2) {
    fputs ("Invalid command  line\n", stderr);
    return 1;
  }

  fp = fopen(argv[1], "rb");
  if (fp == NULL) {
    fputs ("Error opening image file\n", stderr);
    return 1;
  }
  ctx = wire_sim_read_image (fp)
  if (ctx == NULL) {
    fputs ("Error reading image file\n", stderr);
    return 1;
  }
  fclose (fp);

  if (!hw_resume (ctx)) {
    fputs ("Error resuming hardware.\n", stderr);
    retval = 1;
    goto cleanup;
  }

  while (1) {
    if (0) {
      /* If writing a snapshot fails this time, we'll try again on the
	 next cycle.  */
      wire_sim_write_snapshot (ctx);
    }
    if (!hw_step (ctx)) {
      fputs ("Hardware simulation error.\n", stderr);
      retval = 1;
      break;
    }
    sim_time++;
  }

 cleanup:
  hw_suspend (ctx);
  wire_sim_free_image (ctx);
  return retval;
}
