#pragma once

#include <mockturtle/networks/aig.hpp>
#include <mockturtle/views/fanout_view.hpp>
#include <mockturtle/algorithms/cut_enumeration.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/utils/progress_bar.hpp>
#include <mockturtle/utils/node_map.hpp>

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
      std::cout << "[📊 Fanout Info] Node " << ntk.node_to_index(n) 
                << " original fanout = " << fanout << "\n";
    }


    // get cutsc
    const auto fanin = ntk.fanin_size(n);
    const auto& node_cuts = cuts.cuts(ntk.node_to_index(n));
    bool replaced = false;
    for (const auto& cut : node_cuts)
    {
      if (cut->size() < fanin)
        continue;

      std::vector<signal<Ntk>> children;
      for (auto idx : *cut)
        children.push_back(ntk.make_signal(ntk.index_to_node(idx)));

      // auto dummy = ntk.get_constant(true);
      // children[0] = ntk.create_and(children[0], dummy); //
      // auto new_node = ntk.create_and(children[0], children[1]);

      // auto new_node = ntk.clone_node(ntk, n, children);
      auto new_signal = ntk.clone_node(ntk, n, children);

      
      std::vector<node<Ntk>> fanouts;
      fanout_ntk.foreach_fanout(n, [&](auto const& f) {
        fanouts.push_back(f);
      });

      size_t half = fanouts.size() / 2;
      for (size_t j = 0; j < half; ++j)
      {
        ntk.replace_in_node(fanouts[j], n, new_signal);
      }

      if (ps.show_FO)
      {
        fanout_view<Ntk> updated_fanout_ntk{ntk};
        std::cout << "[<-> Replacement] Node " << ntk.node_to_index(n)
                  << " updated fanouts:\n";
        std::cout << "  ├─ Original node fanout now = "
                  << updated_fanout_ntk.fanout_size(n) << "\n";
        std::cout << "  └─ New duplicated node fanout = "
                  << updated_fanout_ntk.fanout_size(ntk.get_node(new_signal)) << "\n";
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

} 
