/* Main page comment for Doxygen.

Copyright (C) 2011, 2012, 2013 Andrew Makousky
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.  */

/**
 * @mainpage Slider Wave Editor Architecture
 *
 * Slider's source code makes a central distinction between two
 * different facilities: the user interface and the internal data
 * model.  The following information contains some discussion on each
 * of those topics.  Make sure you've read the user documentation and
 * are familiar with how Slider works; otherwise, you'll have a bit of
 * trouble understanding the developer documentation.
 *
 * @section uiexpl_sec The User Interface
 * Since this program was primarily intended to be interactive, the
 * existence of the user interface is practically the most important
 * part of the program.  In fact, the only reason for the existence of
 * the data model back-end was so that this program could be made
 * useful, of course: so that you could export your data to other
 * <i>real</i> audio programs.
 *
 * The program's user interface makes extensive use of a widget
 * combination called a scientific notation editor.  Ideally, this
 * should be implemented as a GTK+ widget class, but right now, the
 * source code implements each part of the ideal widget as a separate
 * widget.
 *
 * This paragraph is additional commentary, which you may skip.  One
 * feature of this program's user interface that makes it unusual and
 * probably un-user-friendly is its utilization of widgets that are
 * dynamically created and destroyed in an unconventional way.  This
 * unconventional widget policy also leads to slightly less
 * conventional practices regarding the programming of its user
 * interface.  Originally, the idea of this program's user interface
 * was drawn out in Inkscape, then later created within Glade.  The
 * Glade source files are included within this source distribution.
 * The conventional editing facilities of Glade made it not very
 * suitable to store this dynamic user interface from within an XML
 * file then load that data from the file.  So instead, a slightly
 * older version of Glade was used which had the feature of being able
 * to generate C code that builds the user interface.  This code was
 * then modified as necessary.  Because of the modifications, you
 * should not try to regenerate the code from the Glade project file
 * unless you know what you are doing.
 *
 * The editing area contains one window that is used to edit the
 * fundamental frequency, and a series of one or more other windows
 * that can be used to edit harmonics.  Both such types of windows are
 * generated from a single function, create_wvedit_holder(), and the
 * parameter @a first_type specifies if a fundamental editor should be
 * returned.
 *
 * On the top of the main window is a combo box that is used to select
 * the current fundamental frequency being edited.  When a different
 * fundamental frequency is selected, all of the windows currently in
 * the editing area are destroyed and new windows corresponding to the
 * selected fundamental set are created.  Data is stored within the
 * program's data model to save the state of the previous editor
 * windows.
 *
 * Moving on from here, you should be able to look at the source code
 * in interface.h and wv_editors.h for the implementation of the user
 * interface.
 *
 * The following functions from wv_editors.h are relevant to the user
 * interface: free_slider_data(), add_wv_editor(), remove_wv_editor(),
 * restore_prec_sliders(), select_fund_freq(), unselect_fund_freq().
 *
 * @section datamod_sec The Data Model
 * The program has one global variable which stores all information
 * related to the waveform data within the program: ::wv_all_freqs.
 * This is implemented as a wrapper type to a GArray.
 *
 * The reason why wrapper types are used in this program is to
 * replace ugly syntax such as this:
 *
@verbatim
cur_fund = &g_array_index (wv_all_freqs, Wv_Fund_Freq, i);
cur_harmonic = &g_array_index (cur_fund->harmonics, Wv_Data, j);
cur_harmonic->harmc_num;
@endverbatim
 *
 * with that:
 *
@verbatim
wv_all_freqs->d[i].harmonics->d[j].harmc_num;
@endverbatim
 *
 * Since the program can have only one fundamental frequency selected
 * at a time, there is another global variable which stores the index
 * of the currently selected fundamental set, ::g_fund_set.
 * Fundamental frequencies inside of the C code are always counted as
 * zero-based indices, even though the user interface displays the
 * numbers starting from one.
 *
 * Many of the functions take a parameter to specify which fundamental
 * frequency they should work with.  The reason why these functions
 * have this behavior rather than using ::g_fund_set is because it
 * allows them to work with a fundamental frequency other than the one
 * which is currently selected.  This is important for file loading.
 *
 * Moving on from here, you should be able to look at the source code
 * in the rest of this program.  I hope you found this document
 * useful.
 */
