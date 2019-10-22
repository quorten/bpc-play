/* Display SCSI metadata.  */

#include <arpa/inet.h>

#include <stdio.h>

/* SCSI definitions */

/* Commands */
#define SC_INQUIRY 0x12
#define SC_READ_CAPACITY_10 0x25
#define SC_READ_10 0x28

/* Statuses */
#define SC_GOOD 0x00
#define SC_CHECK_CONDITION 0x02
#define SC_CONDITION_MET 0x04
#define SC_BUSY 0x08
#define SC_ACA_ACTIVE 0x30
#define SC_TASK_ABORTED 0x40

#define SC_VENDOR_ID_SIZE 8
#define SC_PRODUCT_ID_SIZE 16
#define SC_REVISION_SIZE 4
#define SC_COMPLETION_TIMEOUT 300

typedef struct  __attribute__((packed))
{
	unsigned char DeviceType;
	unsigned char DeviceQualifier;
	unsigned char Version;
	unsigned char ResponseFormat;
	unsigned char AdditionalLength;
	unsigned char VendorUse1;
	short Reserved1;
	char VendorID[SC_VENDOR_ID_SIZE];
	char ProductID[SC_PRODUCT_ID_SIZE];
	char Revision[SC_REVISION_SIZE];
	unsigned char VendorUse2[20];
	unsigned char Reserved2[42];
	unsigned char VendorUse3[158];
} ScInquiryRecord;

typedef struct  __attribute__((packed))
{
	unsigned int last_lba;
	unsigned int blksize;
} ScReadCapRecord;

typedef struct  __attribute__((packed))
{
	short scsi_id;
	ScInquiryRecord iq_resp;
	ScReadCapRecord read_cap;
} ScMeta;

/* Print out SCSI core metadata in a human-readable format.  */
void
sc_print_meta(meta)
	ScMeta *meta;
{
	unsigned char new_DeviceType;
	unsigned char new_DeviceQualifier;
	new_DeviceType = meta->iq_resp.DeviceType & (32 - 1);
	new_DeviceQualifier = meta->iq_resp.DeviceType >> 5;
	switch (new_DeviceType)
	{
	case 0:
		puts("Direct access block device.");
		break;
	case 1:
		puts("Sequential access device (e.g. tape).");
		break;
	case 2:
		puts("Printer device.");
		break;
	case 3:
		puts("Processor device.");
		break;
	case 4:
		puts("Write-once device (e.g. optical disc).");
		break;
	case 5:
		puts("CD/DVD device.");
		break;
	case 6:
		puts("Scanner device (obsolete).");
		break;
	case 7:
		puts("Optical memory device (e.g. optical disc).");
		break;
	case 8:
		puts("Medium changer device (e.g. jukebox).");
		break;
	case 9:
		puts("Communications device (obsolete).");
		break;
	case 10:
	case 11:
		puts("Graphic arts pre-press device (obsolete).");
		break;
	case 12:
		puts("Storage array controller device (e.g. RAID).");
		break;
	case 13:
		puts("Enclosure services device.");
		break;
	case 14:
		puts("Simplified direct-access device (e.g. disk).");
		break;
	case 15:
		puts("Optical card reader/writer device.");
		break;
	case 16:
		puts("Bridge Controller Commands.");
		break;
	case 17:
		puts("Object-based Storage Device.");
		break;
	case 18:
		puts("Automation/Drive Interface.");
		break;
	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
	case 24:
	case 25:
	case 26:
	case 27:
	case 28:
	case 29:
		puts("Reserved peripheral device type.");
		break;
	case 30:
		puts("Well known logical unit [b].");
		break;
	case 31:
		puts("Unknown or no device type.");
		break;
	default:
		printf("Unknown or no device type 0x%02x.\n",
			   (int)new_DeviceType);
		break;
	}
	switch (new_DeviceQualifier)
	{
	case 0:
		puts("Peripheral device connected or unknown.");
		break;
	case 1:
		puts("No connected peripheral, but supported.");
		break;
	case 2:
		puts("Reserved peripheral qualifier.");
		break;
	case 3:
		puts("Peripheral device not supported.");
		break;
	default:
		printf("Vendor-specific peri. qual. 0x%x.\n",
			   (int)new_DeviceQualifier);
		break;
	}
	printf("Reserved = 0x%02x\n", meta->iq_resp.DeviceQualifier);
	printf("SCSI version = 0x%02x\n", (int)meta->iq_resp.Version);
	printf("Response data format = 0x%02x\n",
		   (int)meta->iq_resp.ResponseFormat);
	if (meta->iq_resp.AdditionalLength > 0)
	{
		unsigned i;
		fputs("VendorID: ", stdout);
		for (i = 0; i < SC_VENDOR_ID_SIZE; i++)
			putchar(meta->iq_resp.VendorID[i]);
		putchar('\n');
		fputs("ProductID: ", stdout);
		for (i = 0; i < SC_PRODUCT_ID_SIZE; i++)
			putchar(meta->iq_resp.ProductID[i]);
		putchar('\n');
		fputs("Revision: ", stdout);
		for (i = 0; i < SC_REVISION_SIZE; i++)
			putchar(meta->iq_resp.Revision[i]);
		putchar('\n');
	}

	/* We get the LBA of the last block, add one to get
	   number of blocks.  */
	printf("lbasize = %u, blksize = %u\n",
		   ntohl(meta->read_cap.last_lba) + 1, ntohl(meta->read_cap.blksize));
}

int
main()
{
	ScMeta meta;
	fread(&meta, sizeof(ScMeta), 1, stdin);
	printf("SCSI ID: %d\n", ntohs(meta.scsi_id));
	sc_print_meta(&meta);
	return 0;
}
