#include <alice/alice.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/io/blif_reader.hpp>
#include <mockturtle/io/write_blif.hpp>
#include <mockturtle/algorithms/cut_rewriting.hpp>
#include <lorina/blif.hpp>

// Command to load a BLIF file
class read_blif_command : public alice::command
{
public:
    read_blif_command() : alice::command(this->env, "read_blif") {
        opts.add_option("filename", filename, "BLIF file to read");
    }

    void execute() override {
        mockturtle::aig_network aig;
        auto result = lorina::read_blif(filename, mockturtle::blif_reader(aig));

        if (lorina::read_blif(filename, mockturtle::blif_reader(aig)) != lorina::return_code::success)
        {
            env->err() << "[e] Error reading BLIF\n";
            return;
        }
        
        store<mockturtle::aig_network>().extend() = aig;

        std::cout << "BLIF file loaded successfully!" << std::endl;
    }

private:
    std::string filename;
};

// Command to apply cut rewriting
class cut_rewriting_command : public alice::command
{
public:
    explicit cut_rewriting_command(const alice::environment::ptr &env) : alice::command(env, "Performs cut rewriting optimization on AIG.")
    {
    }

    void execute() override
    {
        auto &aig = store<mockturtle::aig_network>().current();

        mockturtle::cut_rewriting_params ps;
        ps.cut_enumeration_ps.cut_size = 4;

        // Perform rewriting using AIG resynthesis
        mockturtle::xag_npn_resynthesis resyn;
        aig = mockturtle::cut_rewriting(aig, resyn, ps);
        
        // Optional cleanup after rewriting
        aig = mockturtle::cleanup_dangling(aig);
    }
};


// Command to write the optimized AIG as BLIF
class write_blif_command : public alice::command
{
public:
    write_blif_command() : alice::command(this->env, "write_blif") {
        opts.add_option("filename", filename, "Output BLIF file");
    }

    void execute() override {
        auto& aig = store<mockturtle::aig_network>().current();
        mockturtle::write_blif(aig, filename);
        std::cout << "Saved optimized circuit to " << filename << std::endl;
    }

private:
    std::string filename;
};

// Register commands
ALICE_ADD_COMMAND(read_blif, "I/O")
ALICE_ADD_COMMAND(cut_rewriting, "Optimization")
ALICE_ADD_COMMAND(write_blif, "I/O")