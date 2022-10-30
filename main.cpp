#include <boost/program_options.hpp>
#include <algorithm> 
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <iomanip>

namespace po = boost::program_options;

std::map<uint32_t, std::vector<uint8_t>> g_memory;

static int hex2bin(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return 0;
}

static void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}
static void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

static void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

int DoConversion(const std::string& binfname, 
                 const std::string& datfname, 
                 const std::string& outfname, 
                 uint32_t wordsize, 
                 uint32_t depth, 
                 uint32_t binaddr, 
                 const std::string& defval) {

    if (wordsize == 0) {
        std::cout << "Error, invalid word size." << std::endl;
        return -1;
    }

    uint32_t out_depth = depth;

    if (!binfname.empty()) {
        std::ifstream inputfile(binfname.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
        if (!inputfile.is_open()) {
            std::cout << "Error opening file." << std::endl;
            return -1;
        }

        size_t file_size = inputfile.tellg();      

        size_t file_depth = (file_size + wordsize - 1) / wordsize;

        std::cout << "Input file content size = " << file_size << " bytes (" << file_depth << " words)." << std::endl;

        uint32_t bin_depth = binaddr + file_depth;

        if (depth != 0 && depth < bin_depth) {
            std::cout << "Error, specified depth too small, should be at least " << bin_depth << "." << std::endl;
            inputfile.close();
            return -1;
        }

        if (bin_depth > out_depth) {
            out_depth = bin_depth;
        }

        std::vector<uint8_t> buffer(file_size);
        inputfile.seekg(0, std::ios::beg);
        inputfile.read((char*)buffer.data(), file_size);        
        inputfile.close();

        g_memory[binaddr] = std::move(buffer);
    }

    if (!datfname.empty()) {
        std::ifstream inputfile(datfname.c_str());
        if (!inputfile.is_open()) {
            std::cout << "Error opening file." << std::endl;
            return -1;
        }
        std::string line;
        uint32_t offset_start, offset(0);
        std::vector<uint8_t> buffer;
        while (std::getline(inputfile, line)) { 
            
            trim(line);

            if (line.rfind("#", 0) == 0)
                continue;
                
            if (line.rfind("@", 0) == 0) { 
                if (!buffer.empty()) {
                    g_memory[offset_start] = buffer;
                    buffer.clear();
                }
                offset = std::atoi(line.c_str() + 1);
                continue;
            };

            int len = line.size();
            if (len == 0) {
                ++offset;
                continue;
            }

            int i = 0;
            uint8_t byte = 0;

            for (int j = len - 1; j >= 0; --j) {
                int val = hex2bin(line[j]);
                byte |= val << (4 * i); 
                if (i == 1) {
                    if (buffer.empty()) {
                        offset_start = offset;
                    }
                    buffer.push_back(byte);
                    byte = 0;
                }
                i = i ^ 1;
            }

            if (byte != 0) {
                buffer.push_back(byte);
                byte = 0;
            }

            for (int j = len/2; j < wordsize; ++j) {
                buffer.push_back(0);
            }

            ++offset;
        }
        
        inputfile.close();
        
        if (!buffer.empty()) {
            g_memory[offset_start] = buffer;
            buffer.clear();
        }

        if (depth != 0 && depth < offset) {
            std::cout << "Error, specified depth too small, should be at least " << offset << "." << std::endl;
            inputfile.close();
            return -1;
        }       
        
        if (offset > out_depth) {
            out_depth = offset;
        }
    }

    std::ofstream outputfile(outfname.c_str(), std::ios::out);
    if (!outputfile.is_open()) {
         std::cout << "Error opening output file." << std::endl;
        return -1;
    }

    std::cout << "Generating COE file: word size = " << wordsize << ", depth = " << depth << std::endl;

    outputfile << "MEMORY_INITIALIZATION_RADIX=16;" << std::endl;
    outputfile << "MEMORY_INITIALIZATION_VECTOR=" << std::endl;   

    uint32_t i = 0;
    for (auto it : g_memory) {
        for (; i < it.first; ++i) {
            if (i) outputfile << "," << std::endl;
            outputfile << defval;
        } 

        uint32_t cur_size = it.second.size();
        uint32_t cur_depth = (cur_size + wordsize - 1) / wordsize;

        for (uint32_t i0 = i, n = i0 + cur_depth; i < n; ++i) {
            if (i) outputfile << "," << std::endl;
            uint32_t j = i - i0;
            for (uint32_t w = 0; w < wordsize; ++w) {
                uint32_t k = (j * wordsize) + (wordsize - w - 1);
                int data = (k < cur_size) ? it.second[k] : 0;
                outputfile << std::hex << std::setw(2) << std::setfill('0') << (int)data;
            }
        }
    }
    for (; i < out_depth; ++i) {
        if (i) outputfile << "," << std::endl;
        outputfile << defval;
    }

    outputfile << ";" << std::endl;
    outputfile.close();

    std::cout << "Output file: " << outfname << std::endl;

    return 0;
}

int main (int argc, char* argv[]) {
    std::cout << "------------------------------------------------" << std::endl;
    std::cout << "Binary to Xilinx COE File Converter." << std::endl;
    std::cout << "------------------------------------------------" << std::endl;

    std::string binfile;
    std::string datafile;
    std::string outfile;
    uint32_t wordsize;
    uint32_t depth;
    uint32_t binaddr;
    std::string defval;
       
    po::options_description opts("Options");
    opts.add_options()
        ("help", "Show command line options.")
        ("binary", po::value<std::string>(&binfile), "Input binary file.")
        ("data", po::value<std::string>(&datafile), "Input data file.")
        ("out", po::value<std::string>(&outfile), "Output file (optional).")
        ("wordsize", po::value<uint32_t>(&wordsize)->default_value(4), "Word size in bytes (default 4).")
        ("depth", po::value<uint32_t>(&depth)->default_value(0), "Address size (optional).")
        ("binaddr", po::value<uint32_t>(&binaddr)->default_value(0), "Binary address (optional).")
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

    if (!vm.count("binary") && !vm.count("data")) {
        std::cout << opts << std::endl;
        return -1;
    }

    if (!vm.count("out")) {
        outfile = "output.coe"; 
    }    
    
    return DoConversion(binfile, datafile, outfile, wordsize, depth, binaddr, defval);
}