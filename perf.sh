#!/bin/bash
#PERF="perf stat -B -e mem_inst_retired.all_loads,mem_inst_retired.all_stores,L1-dcache-loads,L1-dcache-stores,mem-loads,mem-stores,cache-references,cache-misses,LLC-loads,LLC-load-misses,LLC-stores,LLC-store-misses,l2_rqsts.all_demand_references,l2_rqsts.all_demand_miss,cycles,instructions,page-faults,cpu-migrations,context-switches,l2_rqsts.rfo_hit,l2_rqsts.rfo_miss"
PERF="perf stat -B -e cache-references,cache-misses,l2_rqsts.references,l2_rqsts.miss"
${PERF} "$@"

