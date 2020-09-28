#include "FelixFormat.hh"
#include "PdspChannelMapService.h"
#include "FrameFile.h"

#include <fstream>
#include <iostream>
#include <map>
#include <cstdio>
#include <string.h>

#include <inttypes.h> // For PRIx64 format specifier

#include "CLI11.hpp"

//======================================================================
void print_channel_numbers(FILE* f, const dune::FelixFrame* frame)
{
    PdspChannelMapService channelMapService;
    fprintf(f, "0x0               ");
    for(int i=0; i<256; ++i){
        fprintf(f, "% 6d ", channelMapService.getOfflineChannel(frame, i));
    }
    fprintf(f, "\n");
            
}
//======================================================================
//
// dumpfile-to-text: converts raw binary long-readout FELIX dumps to a
// text format. See --help for options
//
// The output format is:
//
// 0x0            ch0     ch1     ch2 ...
// timestamp0    adc0    adc1    adc2 ...
// timestamp1    adc0    adc1    adc2 ...
// ...

// where ch0, ch1 etc are the offline channel numbers (in
// "electronics" order) and timestampX is the timestamp in 50 MHz
// clock ticks. In this scheme, each column is the waveform from one
// channel
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
    std::string b33ext=".33b";
    std::string input_file_name_str = std::string(input_file_name.c_str());
    std::string b33string=input_file_name_str.append(b33ext);
    std::cout<<"b33 file name: "<<b33string<<std::endl;
    // We use FILE* and fprintf, instead of std::ofstream, for output so we can get nice columns
    FILE* fout=fopen(output_file_name.c_str(), "w");
    FILE* foutb33=fopen(b33string.c_str(), "w");

    uint64_t prev_timestamp=0;
    size_t nbad=0;
    // The number of frames we'll actually loop over: might have been limited by the -n option
    size_t nframes=(nticks==-1) ? frame_file.num_frames() : std::min((size_t)nticks, frame_file.num_frames());
    bool inHitsRegion=false;

    for(size_t i=0; i<nframes; ++i){
        const dune::FelixFrame* frame=frame_file.frame(i);
        // Print the header
        if(i==0){
            print_channel_numbers(fout, frame);
        }
        // Print the ADC value for each of the 256 channels in the frame
        uint64_t timestamp=frame->timestamp();
        if(firsthittimestamp<=timestamp){
            inHitsRegion=true;
        }
        if((lasthittimestamp+(63*25))<=timestamp){
            inHitsRegion=false;
        }
        if(inHitsRegion==1){
            size_t n_32b_words=sizeof(dune::FelixFrame)/4;
            //uint32_t binaryframe[n_32b_words]={0};
            uint32_t binaryframe[116]={0};
            frame_file.frame_binary(i, binaryframe);
            /*
            const char* binaryframe=frame_file.frame_binary(i);
            //work out number of 32b values which will be printed
            int n_32b_words = strlen(binaryframe)/4;
            //sizeof(dune::FelixFrame)
            std::cout<<"n_32b_words in ff: "<<n_32b_words<<std::endl;
            for(int i=0; i<n_32b_words;++i){
                uint8_t byte1=binaryframe[(i*4)+0];
                uint8_t byte2=binaryframe[(i*4)+1];
                uint8_t byte3=binaryframe[(i*4)+2];
                uint8_t byte4=binaryframe[(i*4)+3];
                uint32_t wordbytes=byte4 | (byte3<<8) | (byte2<<16) | (byte1<<24);
                std::cout<<"32b hex line: "<<"%#08"<<PRIx32<<wordbytes<<" 1\n"<<std::endl;
                fprintf(foutb33, "%#08" PRIx32 " 1\n",wordbytes);
                return 1;
            }
            */
           for(int j=0;j<n_32b_words;++j){
               std::cout<<"32b hex line: "<<"%#08" PRIx32<<binaryframe[j]<<" 1\n"<<std::endl;
               fprintf(foutb33, "%#08" PRIx32 " 1\n", binaryframe[j]);
            }
            
            fprintf(fout, "%#" PRIx64 " ", frame->timestamp());
            for(int j=0; j<256; ++j){
                fprintf(fout, "% 6d ", frame->channel(j));
            }
            fprintf(fout, "\n");
        }
        // Check that the gap between timestamps is 25 ticks
        if(prev_timestamp!=0 && (timestamp-prev_timestamp!=25)){
            std::cerr << "Inter-frame timestamp gap of " << (timestamp-prev_timestamp) << " ticks at ts 0x" << std::hex << timestamp << std::dec << ". index=" << i << std::endl;
            ++nbad;
            if(inHitsRegion==1){
                std::cerr<<"This occurs within the region of overlapping hits and adcs!\n";
            }
        }
        prev_timestamp=timestamp;
    }
    std::cout << nbad << " bad of " << nframes << std::endl;
    std::cout << "Size of felix frame in bytes: " << sizeof(dune::FelixFrame) << std::endl;
}
