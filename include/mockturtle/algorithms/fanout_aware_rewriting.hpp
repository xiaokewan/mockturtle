#pragma once

#include <mockturtle/networks/aig.hpp>
#include <mockturtle/views/fanout_view.hpp>
#include <mockturtle/algorithms/cut_enumeration.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/utils/progress_bar.hpp>

namespace mockturtle
{

struct fanout_aware_params
{
  uint32_t max_fanout = 4;
  bool verbose = false;
  bool show_FO = false;
};

template<typename Ntk>
void fanout_aware_rewriting(Ntk& ntk, fanout_aware_params const& ps = {})
{
  fanout_view fanout_ntk{ntk};
  const auto cuts = cut_enumeration<Ntk>(ntk);
  int skipped_nodes = 0;

  progress_bar pbar{ntk.size(), "fanout_aware_rewriting |{0}| node = {1:>4}/{2}"};

  ntk.foreach_gate([&](auto const& n, auto i) {
    pbar(i, i, ntk.size());

    const auto fanout = fanout_ntk.fanout_size(n);
    if (fanout <= ps.max_fanout)
      return;

    if (ps.show_FO)
    {
      std::cout << "[！ Fanout Info] Node " << ntk.node_to_index(n) 
                << " original fanout = " << fanout << "\n";
    }

    const auto fanin = ntk.fanin_size(n);
    const auto& node_cuts = cuts.cuts(ntk.node_to_index(n));
    bool replaced = false;

    for (const auto& cut : node_cuts)
    {
      if (cut->size() < fanin)
        continue;

      // from cut obtain fanin signals
      std::vector<signal<Ntk>> children;
      for (auto idx : *cut)
        children.push_back(ntk.make_signal(ntk.index_to_node(idx)));

      // clone node n to a signal
      auto new_signal = ntk.clone_node(ntk, n, children);
      // aviod strutural hashing

      // force create node
      // auto tmp = ntk.create_buf(children[0]);
      // auto new_signal = ntk.create_and(tmp, children[1]);
      

      if (ntk.get_node(new_signal) == n)
      {
        std::cout << "⚠️ Still same node, no duplication!\n";
      }
      
      // obtain fanouts
      std::vector<node<Ntk>> fanouts;
      fanout_ntk.foreach_fanout(n, [&](auto const& f) {
        fanouts.push_back(f);
      });


      const auto prev_fanout = fanouts.size();
      size_t half = fanouts.size() / 2;
      size_t actual_replaced = 0;
      signal<Ntk> orig_sig = ntk.make_signal(n);

      for (size_t j = 0; j < half; ++j)
      {
        std::vector<signal<Ntk>> old_fanins;
        ntk.foreach_fanin(fanouts[j], [&](auto const& s, auto i) {
          old_fanins.push_back(s);
        });
      
        bool found_fanin = false;
        ntk.foreach_fanin(fanouts[j], [&](auto const& s, auto i) {
          if (ntk.get_node(s) == n)
          {
            found_fanin = true;
          }
        });
      
        if (found_fanin)
        {
          ntk.replace_in_node_no_restrash(fanouts[j], n, new_signal);
          fanout_ntk.notify_node_modified_manually(fanouts[j], old_fanins);
          ++actual_replaced;
        }
        else
        {
          std::cout << "⚠️ [WARN] No replacement happened for fanout "
                    << ntk.node_to_index(fanouts[j])
                    << " of node " << ntk.node_to_index(n) << "\n";
        }
      }
      

      if (ps.show_FO)
      {
        // fanout_view<Ntk> updated_fanout_ntk{ntk};
        // updated_fanout_ntk.update_fanout();
        fanout_ntk.update_fanout();
        std::cout << "[<-> Replacement] Node " << ntk.node_to_index(n)
                  << " updated fanouts:\n";
        std::cout << "  ├─ Original node fanout " << ntk.node_to_index(n) << ": = "
                  << prev_fanout << "\n";

        std::cout << "  ├─ Original fanouts of node " << ntk.node_to_index(n) << ": [ ";
        for (auto const& f : fanouts)
          std::cout << ntk.node_to_index(f) << " ";
        std::cout << "]\n";

        std::cout << "  ├─ Original node fanout now "  << ntk.node_to_index(n) << ": = "
                  << fanout_ntk.fanout_size(n) << "\n";
                  
        std::cout << "  ├─ Fanouts of original node " << ntk.node_to_index(n) << ": [ ";
        fanout_ntk.foreach_fanout(n, [&](auto const& f) {
          std::cout << ntk.node_to_index(f) << " ";
        });
        std::cout << "]\n"; 
        
        std::cout << "  └─ New duplicated node fanout " << ntk.node_to_index(ntk.get_node(new_signal)) << ": = "
                  << fanout_ntk.fanout_size(ntk.get_node(new_signal)) << "\n";
                
        std::cout << "  └─ Fanouts of duplicated node " << ntk.node_to_index(ntk.get_node(new_signal)) << ": [ ";
        fanout_ntk.foreach_fanout(ntk.get_node(new_signal), [&](auto const& f) {
          std::cout << ntk.node_to_index(f) << " ";
        });
        std::cout << "]\n";
      }

      replaced = true;
      break;
    }

    if (!replaced)
    {
      ++skipped_nodes;
      std::cout << "[! Warning] Node " << ntk.node_to_index(n)
                << " skipped | fanout = " << fanout
                << ", fanin = " << fanin
                << ", #cuts = " << node_cuts.size() << "\n";
    }
  });

  ntk = cleanup_dangling(ntk);

  if (ps.verbose)
  {
    std::cout << "\n[ℹ️ Info] Skipped high-fanout nodes (not replaced): " << skipped_nodes << std::endl;
  }
}

} // namespace mockturtle
