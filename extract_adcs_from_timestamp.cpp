#include "FelixFormat.hh"
#include "PdspChannelMapService.h"
#include "FrameFile.h"

#include <fstream>
#include <iostream>
#include <map>
#include <cstdio>

#include <inttypes.h> // For PRIx64 format specifier

#include "CLI11.hpp"

//======================================================================
//
// converts raw binary adc data to a text format interpretable by 
// dataflow-software tools (to read in to wib2g frames). Output is 1 wib
// frame per line
// The output format is:
// 0slot, 1crate 2fiber 3version 4errors 5oos 6mm 7timestamp 8counter 9coldata0_chksma
// 10coldata0_chksmb 11coldata0_errorregister 12coldata0_streamerr1 13coldata0_streamerr2
// 14coldata0_convertcount 15coldata0_hdr1 ... 22coldata0_hdr8 23coldata1... 37coldata2... ... 65coldata4... 
// ... 79adc0 ... 334adc255

int main(int argc, char** argv)
{
    // -----------------------------------------------------------------
    // Parse the command-line arguments
    CLI::App app{"Print dumped hits"};

    std::string input_file_name;
    app.add_option("-i", input_file_name, "Input file", true)->required();

    std::string output_file_name;
    app.add_option("-o", output_file_name, "Output file", true)->required();

    uint64_t firsthittimestamp;
    app.add_option("-f", firsthittimestamp, "First timestamp (dec) of hits", true)->required();

    uint64_t lasthittimestamp;
    app.add_option("-l", lasthittimestamp, "Last timestamp (dec) of hits", true)->required();

    int nticks=-1;
    app.add_option("-n", nticks, "Number of ticks to output (-1 for no limit)", true);

    CLI11_PARSE(app, argc, argv);

    // -----------------------------------------------------------------

    // The size in bytes of a frame
    constexpr size_t frame_size=sizeof(dune::FelixFrame);

    FrameFile frame_file(input_file_name.c_str());
    // We use FILE* and fprintf, instead of std::ofstream, for output so we can get nice columns
    // FILE* fout=fopen(output_file_name.c_str(), "w");

    //we use ofstream because we don't care about reabable columns
    std::ofstream ofs(output_file_name, std::ofstream::out);

    uint64_t prev_timestamp=0;
    uint64_t last_kept_timestamp=lasthittimestamp+(64*25);
    size_t nbad=0;
    // The number of frames we'll actually loop over: might have been limited by the -n option
    size_t nframes=(nticks==-1) ? frame_file.num_frames() : std::min((size_t)nticks, frame_file.num_frames());

    bool inHitsRegion=false;
    for(size_t i=0; i<nframes; ++i){
        const dune::FelixFrame* frame=frame_file.frame(i);

        uint64_t timestamp=frame->timestamp();
        if(timestamp==firsthittimestamp){
            inHitsRegion=true;
        }
        else if(timestamp==lasthittimestamp){
            inHitsRegion=false;
        }
// 0slot, 1crate 2fiber 3version 4errors 5oos 6mm 7timestamp 8counter 9coldata0_chksma
// 10coldata0_chksmb 11coldata0_errorregister 12coldata0_streamerr1 13coldata0_streamerr2
// 14coldata0_convertcount 15coldata0_hdr1 ... 22coldata0_hdr8 23coldata1... 37coldata2... ... 65coldata4... 
// ... 79adc0 ... 334adc255
        if(inHitsRegion){
            ofs<<frame->slot_no()<<" "
            <<frame->crate_no()<<" "
            <<frame->fiber_no()<<" "
            <<frame->version()<<" "
            <<frame->wib_errors()<<" "
            <<frame->oos()<<" "
            <<frame->mm()<<" "
            <<frame->timestamp()<<" "
            <<frame->wib_counter()<<" ";
            for(uint8_t j; j<4 ;++j){
                ofs<<frame->checksum_a(j)<<" "
                <<frame->checksum_b(j)<<" "
                <<frame->error_register(j)<<" "
                <<frame->s1_error(j)<<" "
                <<frame->s2_error(j)<<" "
                <<frame->coldata_convert_count(j)<<" "
                <<frame->hdr(j,0)<<" "
                <<frame->hdr(j,1)<<" "
                <<frame->hdr(j,2)<<" "
                <<frame->hdr(j,3)<<" "
                <<frame->hdr(j,4)<<" "
                <<frame->hdr(j,5)<<" "
                <<frame->hdr(j,6)<<" "
                <<frame->hdr(j,7)<<" ";
            }
            for(uint8_t j; j<256; ++j){
                ofs<<frame->channel(j)<<" ";
            }
            ofs<<"\n";
        }

        // Check that the gap between timestamps is 25 ticks
        if(prev_timestamp!=0 && (timestamp-prev_timestamp!=25)){
            std::cerr << "Inter-frame timestamp gap of " << (timestamp-prev_timestamp) << " ticks at ts 0x" << std::hex << timestamp << std::dec << ". index=" << i << std::endl;
            ++nbad;
        }
        prev_timestamp=timestamp;
    }
    std::cout << nbad << " bad of " << nframes << std::endl;
}
