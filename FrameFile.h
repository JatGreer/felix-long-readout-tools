#ifndef FRAMEFILE_H
#define FRAMEFILE_H

#include <fstream>
#include <iostream>
#include <stdexcept> // For std::runtime_error
#include "FelixFormat.hh"

class FrameFile
{
public:

    FrameFile(const char* filename)
        : m_file(filename, std::ifstream::binary),
          m_buffer(new char[sizeof(dune::FelixFrame)])
    {
        if(m_file.bad() || m_file.fail() || !m_file.is_open()){
            throw std::runtime_error(std::string("Bad file ")+std::string(filename));
        }
        // Calculate the length of the file
        m_file.seekg(0, m_file.end);
        m_length = m_file.tellg();
        m_file.seekg(0, m_file.beg);
        if(m_length==0){
            throw std::runtime_error("Empty file");
        }
        if(m_length%sizeof(dune::FelixFrame)!=0){
            throw std::runtime_error("File does not contain an integer number of frames");
        }
        m_n_frames=m_length/sizeof(dune::FelixFrame);
    }

    ~FrameFile()
    {
        m_file.close();
        delete[] m_buffer;
    }

    // Length of the file in bytes
    size_t length() const {return m_length;}
    // Number of frames in the file
    size_t num_frames() const { return m_n_frames; }

    // Read the ith frame into the buffer and return a pointer to the
    // first frame in the fragment. This object owns the memory:
    // subsequent calls will overwrite the buffer with a different
    // fragment
    const dune::FelixFrame* frame(size_t i)
    {
        if(i>=num_frames()) return nullptr;
        // Seek to the right place in the file
        m_file.seekg(i*sizeof(dune::FelixFrame));
        // Check we didn't go past the end
        if(m_file.bad() || m_file.eof()) return nullptr;
        // Actually read the fragment into the buffer
        m_file.read(m_buffer,sizeof(dune::FelixFrame));
        if(m_file.bad() || m_file.eof()) return nullptr;
        return reinterpret_cast<dune::FelixFrame*>(m_buffer);
    }
    // Do the same but this time only extract
    // the uninterpreted binary
    const char* frame_binary(size_t i)
    {
        if(i>=num_frames()) return nullptr;
        // Seek to the right place in the file
        m_file.seekg(i*sizeof(dune::FelixFrame));
        // Check we didn't go past the end
        if(m_file.bad() || m_file.eof()) return nullptr;
        // Actually read the fragment into the buffer
        m_file.read(m_buffer,sizeof(dune::FelixFrame));
        if(m_file.bad() || m_file.eof()) return nullptr;
        return m_buffer;
    }

protected:
    std::ifstream m_file;
    char* m_buffer;
    size_t m_length;
    size_t m_n_frames;
};


#endif
