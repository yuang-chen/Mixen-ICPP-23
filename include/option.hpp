
#include <boost/program_options.hpp>
#include "global.hpp"


unsigned params::subgraph_size = (1024 * 1024)/sizeof(unsigned);//(256*1024)/sizeof(float); //512kB cluster size is for cluster constructing
unsigned params::overflow_ceil = 0;
unsigned params::threads = 0;
unsigned params::iters = 20;
unsigned params::rounds = 0;
unsigned params::root_vertex = MASK::MAX_UINT;
bool params::is_dynamic = false;
bool params::is_filter = false;
std::string params::input_file = "";
std::string params::output_file = "";

void options(unsigned argc, char **argv) {
    /////////
    // initialize the parameters here
    /////////
    using namespace boost::program_options;
    options_description desc{"Options"};
    desc.add_options() 
        ("data,d", value<std::string>()->default_value(""), "input data path")
        ("output,o", value<std::string>()->default_value(""), "output data path")

        ("size,s", value<unsigned>()->default_value(256), "subgraph size")
        ("round,r", value<unsigned>()->default_value(3), "rounds")
        ("thread,t", value<unsigned>()->default_value(20), "threads")
        ("vertex,v", value<unsigned>()->default_value(MASK::MAX_UINT), "root vertex")
        ("iter,i", value<unsigned>()->default_value(20), "iterations")
        ("is_dynamic,y", value<bool>()->default_value(false), "dynamic split or not (1|0)")
        ("is_filter,f", value<bool>()->default_value(true), "computational filtering or not (1|0)");

    
    boost::program_options::variables_map vm;
    try
    {
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
        boost::program_options::notify(vm);
    } catch (error& e) {
        std::cerr << "ERROR: " << e.what() << '\n' << '\n' << desc << '\n';
        exit(1);
    }

    params::input_file = vm["data"].as<std::string>();
    if (params::input_file.empty()) {
        std::cout << desc << '\n';
        exit(1);
    } 
    params::output_file = vm["output"].as<std::string>();
    params::subgraph_size = vm["size"].as<unsigned>();
    params::iters = vm["iter"].as<unsigned>();
    params::rounds = vm["round"].as<unsigned>();
    params::threads  = vm["thread"].as<unsigned>();
    params::root_vertex = vm["vertex"].as<unsigned>();
    params::is_dynamic = vm["is_dynamic"].as<bool>();
    params::is_filter = vm["is_filter"].as<bool>();

    std::cout << "--------experimental setting--------"<<'\n';
    std::cout << "is_dynamic: " << BoolToString(params::is_dynamic) 
              << ", is_filtering: " << BoolToString(params::is_filter)
              << ", #threads: " <<  params::threads << '\n';
}


