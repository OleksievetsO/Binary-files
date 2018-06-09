#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>


/// Type used to store and retrieve codes.
using CodeType = std::uint16_t;

namespace globals {

/// Dictionary Maximum Size (when reached, the dictionary will be reset)
    const CodeType dms {std::numeric_limits<CodeType>::max()};

} // namespace globals


/**
     * Compresses the contents of `is` and writes the result to `os`.
     *
     * @param [in] is      input stream
     * @param su   Show Usage information
     * @param [out] os     output stream
*/
void compress(std::istream &is, std::ostream &os)
{
    std::map<std::pair<CodeType, char>, CodeType> dictionary;

    // "named" lambda function, used to reset the dictionary to its initial contents
    const auto reset_dictionary = [&dictionary] {
        dictionary.clear();

        const long int minc = std::numeric_limits<char>::min();
        const long int maxc = std::numeric_limits<char>::max();

        for (long int c = minc; c <= maxc; ++c)
        {
            // to prevent Undefined Behavior, resulting from reading and modifying
            // the dictionary object at the same time
            const CodeType dictionary_size = dictionary.size();

            dictionary[{globals::dms, static_cast<char> (c)}] = dictionary_size;
        }
    };

    reset_dictionary();

    CodeType i {globals::dms}; // Index
    char c;

    while (is.get(c))
    {
        // dictionary's maximum size was reached
        if (dictionary.size() == globals::dms)
            reset_dictionary();

        if (dictionary.count({i, c}) == 0)
        {
            // to prevent Undefined Behavior, resulting from reading and modifying
            // the dictionary object at the same time
            const CodeType dictionary_size = dictionary.size();

            dictionary[{i, c}] = dictionary_size;
            os.write(reinterpret_cast<const char *> (&i), sizeof (CodeType));
            i = dictionary.at({globals::dms, c});
        }
        else
            i = dictionary.at({i, c});
    }

    if (i != globals::dms)
        os.write(reinterpret_cast<const char *> (&i), sizeof (CodeType));
}


/**
     * Decompresses the contents of `is` and writes the result to `os`.
     *
     * @param [in] is      input stream
     * @param su   Show Usage information
     * @param [out] os     output stream
*/
void decompress(std::istream &is, std::ostream &os)
{
    std::vector<std::pair<CodeType, char>> dictionary;

    // "named" lambda function, used to reset the dictionary to its initial contents
    const auto reset_dictionary = [&dictionary] {
        dictionary.clear();
        dictionary.reserve(globals::dms);

        const long int minc = std::numeric_limits<char>::min();
        const long int maxc = std::numeric_limits<char>::max();

        for (long int c = minc; c <= maxc; ++c)
            dictionary.push_back({globals::dms, static_cast<char> (c)});
    };

    const auto rebuild_string = [&dictionary](CodeType k) -> std::vector<char> {
        std::vector<char> s; // String

        while (k != globals::dms)
        {
            s.push_back(dictionary.at(k).second);
            k = dictionary.at(k).first;
        }

        std::reverse(s.begin(), s.end());
        return s;
    };

    reset_dictionary();

    CodeType i {globals::dms}; // Index
    CodeType k; // Key

    while (is.read(reinterpret_cast<char *> (&k), sizeof (CodeType)))
    {
        // dictionary's maximum size was reached
        if (dictionary.size() == globals::dms)
            reset_dictionary();

        if (k > dictionary.size())
            throw std::runtime_error("invalid compressed code");

        std::vector<char> s; // String

        if (k == dictionary.size())
        {
            dictionary.push_back({i, rebuild_string(i).front()});
            s = rebuild_string(k);
        }
        else
        {
            s = rebuild_string(k);

            if (i != globals::dms)
                dictionary.push_back({i, s.front()});
        }

        os.write(&s.front(), s.size());
        i = k;
    }

    if (!is.eof() || is.gcount() != 0)
        throw std::runtime_error("corrupted compressed file");
}

/**
     * Prints usage information and a custom error message.
     *
     * @param s    custom error message to be printed
     * @param su   Show Usage information
     * @return void
*/
void print_usage(const std::string &s = "", bool su = true)
{
    if (!s.empty())
        std::cerr << "\nERROR: " << s << '\n';

    if (su)
    {
        std::cerr << "\nUsage:\n";
        std::cerr << "\tprogram --flag input_file output_file.lzw\n\n";
        std::cerr << "Where `flag' is either `compress' for compressing, or `decompress' for decompressing, and\n";
        std::cerr << "`input_file' and `output_file' are distinct files.\n\n";
        std::cerr << "Examples:\n";
        std::cerr << "\tmegalzw.exe --compress input.bmp output_file.lzv\n";
        std::cerr << "\tmegalzw.exe --dcompress input_file.lzw output_file.bmp\n";
    }

    std::cerr << std::endl;
}

/**
 *  Actual program entry point.
 *
 * @param argc      number of command line arguments
 * @param argv      array of command line arguments
 * @return          EXIT_FAILURE    for failed operation
 * @return          EXIT_SUCCESS    for successful operation
 *
 */
int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_usage("Wrong number of arguments.");
        return EXIT_FAILURE;
    }
    enum class Mode {
        Compress,
        Decompress
    };


    Mode m;

    if (std::string(argv[1]) == "--compress")
        m = Mode::Compress;
    else
    if (std::string(argv[1]) == "--dcompress")
        m = Mode::Decompress;
    else
    {
        print_usage(std::string("flag `") + argv[1] + "' is not recognized.");
        return EXIT_FAILURE;
    }


    const std::size_t buffer_size {1024 * 1024};

    // these custom buffers should be larger than the default ones
    const std::unique_ptr<char[]> input_buffer(new char[buffer_size]);
    const std::unique_ptr<char[]> output_buffer(new char[buffer_size]);

    std::ifstream input_file;
    std::ofstream output_file;

//    input_file.rdbuf()->pubsetbuf(input_buffer.get(), buffer_size);
    input_file.open(argv[2], std::ios_base::binary);

    if (!input_file.is_open())
    {
        print_usage(std::string("input_file `") + argv[2] + "' could not be opened.");
        return EXIT_FAILURE;
    }

//    output_file.rdbuf()->pubsetbuf(output_buffer.get(), buffer_size);
    output_file.open(argv[3], std::ios_base::binary);

    if (!output_file.is_open())
    {
        print_usage(std::string("output_file `") + argv[3] + "' could not be opened.");
        return EXIT_FAILURE;
    }


    try
    {
        input_file.exceptions(std::ios_base::badbit);
        output_file.exceptions(std::ios_base::badbit | std::ios_base::failbit);

        if (m == Mode::Compress) {
            compress(input_file, output_file);

            input_file.close();
            output_file.close();

            input_file.open(argv[2], std::ios_base::binary);
            input_file.seekg(0, std::ios_base::end);
            unsigned long InSize = input_file.tellg();
            input_file.close();

            input_file.open(argv[3], std::ios_base::binary);
            input_file.seekg(0, std::ios_base::end);
            unsigned long OutSize = input_file.tellg();

            std::cout << "The file " << argv[2] << " is compressed by  " << 100 - InSize * 10 / OutSize << "%\n";
        }
        else
        if (m == Mode::Decompress) {
            decompress(input_file, output_file);
            std::cout << "The file " << argv[2] << " is decompressed."  << "\n";
        }
    }
    catch (const std::ios_base::failure &f)
    {
        print_usage(std::string("File input/output failure: ") + f.what() + '.', false);
        return EXIT_FAILURE;
    }
    catch (const std::exception &e)
    {
        print_usage(std::string("Caught exception: ") + e.what() + '.', false);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}