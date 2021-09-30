#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>

namespace po = boost::program_options;

int DoConversion(const std::string& ifname, 
                 const std::string& ofname, 
                 uint32_t word, 
                 uint32_t depth, 
                 uint32_t offset, 
                 const std::string& defval) {

    if (word == 0) {
        std::cout << "Error, invalid word size." << std::endl;
        return -1;
    }
    
    std::ifstream inputfile(ifname.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    if (!inputfile.is_open()) {
        std::cout << "Error opening file." << std::endl;
        return -1;
    }    
     
    size_t file_size = inputfile.tellg();      

    size_t file_depth = (file_size + word - 1) / word;

    std::cout << "Input file content size = " << file_size << " bytes (" << file_depth << " words)." << std::endl;

    uint32_t default_depth = offset + file_depth;

    if (depth == 0) {
        depth = default_depth;
    }

    if (depth < default_depth) {
        std::cout << "Error, specified depth too small, should be at least " << default_depth << "." << std::endl;
        inputfile.close();
        return -1;
    }
    
    std::ofstream outputfile(ofname.c_str(), std::ios::out);
    if (!outputfile.is_open()) {
         std::cout << "Error opening output file." << std::endl;
        inputfile.close();
        return -1;
    }

    std::cout << "Generating COE file: word size = " << word << ", depth = " << depth << std::endl;

    outputfile << "MEMORY_INITIALIZATION_RADIX=16;" << std::endl;
    outputfile << "MEMORY_INITIALIZATION_VECTOR=" << std::endl;

    std::vector<uint8_t> buffer(file_size);

    inputfile.seekg(0, std::ios::beg);
    inputfile.read((char*)buffer.data(), file_size);

    uint32_t i = 0;

    for (; i < offset; ++i) {
        if (i) outputfile << "," << std::endl;
        outputfile << defval;
    }

    for (uint32_t i0 = i, n = i0 + file_depth; i < n; ++i) {
        if (i) outputfile << "," << std::endl;
        uint32_t j = i - i0;
        for (uint32_t w = 0; w < word; ++w) {
            uint32_t k = (j * word) + (word - w - 1);
            int data = (k < file_size) ? buffer[k] : 0;
            outputfile << std::hex << std::setw(2) << std::setfill('0') << (int)data;
        }
    }

    for (; i < depth; ++i) {
        if (i) outputfile << "," << std::endl;
        outputfile << defval;
    }

    outputfile << ";" << std::endl;
    inputfile.close();
    outputfile.close();

    std::cout << "Output file: " << ofname << std::endl;

    return 0;
}

int main (int argc, char* argv[]) {
    std::cout << "------------------------------------------------" << std::endl;
    std::cout << "Binary to Xilinx COE File Converter." << std::endl;
    std::cout << "------------------------------------------------" << std::endl;

    std::string infile;
    std::string outfile;
    uint32_t word;
    uint32_t depth;
    uint32_t offset;
    std::string defval;
       
    po::options_description opts("Options");
    opts.add_options()
        ("help", "Show command line options.")
        ("in", po::value<std::string>(&infile), "Input file (required).")
        ("out", po::value<std::string>(&outfile), "Output file (optional).")
        ("word", po::value<uint32_t>(&word)->default_value(4), "Word size in bytes (default 4).")
        ("depth", po::value<uint32_t>(&depth)->default_value(0), "Address size (optional).")
        ("offset", po::value<uint32_t>(&offset)->default_value(0), "Address offset (optional).")
        ("default", po::value<std::string>(&defval)->default_value("0"), "Default hex value as string (optional).");

    po::variables_map vm;    

    try {        
        po::store(po::parse_command_line(argc, argv, opts), vm);
        po::notify(vm);
    } catch(std::exception& e) {
        std::cout << opts << std::endl;
        std::cerr << e.what() << std::endl;
        return -1;
    }

    if (vm.count("help")) {
        std::cout << opts << std::endl;
        return 0;
    }

    if (!vm.count("in")) {
        std::cout << opts << std::endl;
        return -1;
    }

    if (!vm.count("out")) {
        outfile = infile + ".coe"; 
    }    
    
    return DoConversion(infile, outfile, word, depth, offset, defval);
}