#!/usr/bin/env bash

#
# I MADE THIS SCRIPT WITH AI BECAUSE IT WAS A LOT FASTER THAN DOING IT MYSELF
#
# PLEASE FORGIVE ME
#

#
# bench.sh – continuous time benchmarker
# Compares ./netbookmain (baseline) vs ./netbook (candidate)
#

set -uo pipefail

# ── Colour palette ────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BOLD='\033[1m'; DIM='\033[2m'; RESET='\033[0m'
WHITE='\033[1;37m'; MAGENTA='\033[0;35m'

# ── Accumulators ──────────────────────────────
declare -A sum_val count_val min_val max_val last_val

# ── Run one binary and accumulate stats ───────
run_binary() {
    local bin="$1"
    local raw
    raw=$("./${bin}" --runtime=5 2>&1) || { echo "ERROR: ./${bin} failed" >&2; return 1; }

    local v
    v=$(echo "$raw" | grep -oP 'Total time:\s*\K[0-9]+' | tail -1) || {
        echo "ERROR: could not find 'Total time' in ./${bin} output" >&2
        return 1
    }

    last_val[$bin]="$v"

    if [[ -z "${sum_val[$bin]+x}" ]]; then
        sum_val[$bin]="$v"
        count_val[$bin]=1
        min_val[$bin]="$v"
        max_val[$bin]="$v"
    else
        sum_val[$bin]=$(( sum_val[$bin] + v ))
        count_val[$bin]=$(( count_val[$bin] + 1 ))
        (( v < min_val[$bin] )) && min_val[$bin]="$v"
        (( v > max_val[$bin] )) && max_val[$bin]="$v"
    fi
}

# ── Draw the results ──────────────────────────
draw_table() {
    local run_count="$1"

    clear

    local bm_avg bm_min bm_max bm_last
    local cd_avg cd_min cd_max cd_last

    bm_avg=$(( sum_val[netbookmain] / count_val[netbookmain] ))
    bm_min=${min_val[netbookmain]}
    bm_max=${max_val[netbookmain]}
    bm_last=${last_val[netbookmain]}

    cd_avg=$(( sum_val[netbook] / count_val[netbook] ))
    cd_min=${min_val[netbook]}
    cd_max=${max_val[netbook]}
    cd_last=${last_val[netbook]}

    local delta pct colour sign
    delta=$(( cd_avg - bm_avg ))
    if (( bm_avg != 0 )); then
        pct=$(awk "BEGIN{printf \"%.2f\", (${cd_avg}-${bm_avg})/${bm_avg}*100}")
    else
        pct="0.00"
    fi

    # Lower time = better = green
    if (( delta < 0 )); then
        colour="${GREEN}"; sign=""
    elif (( delta > 0 )); then
        colour="${RED}"; sign="+"
    else
        colour="${WHITE}"; sign=""
    fi

    echo
    printf "${BOLD}${CYAN}  ⚡  netbook time benchmark${RESET}   "
    printf "${DIM}runs completed: ${WHITE}%d${RESET}\n" "$run_count"
    echo

    # ── Summary table ─────────────────────────
    local W=84
    printf '%*s\n' "$W" '' | tr ' ' '═'
    printf "│ %-18s │ %14s │ %14s │ %14s │ %14s │\n" \
        "" "AVG" "MIN" "MAX" "LAST"
    printf '%*s\n' "$W" '' | tr ' ' '─'
    printf "│ ${MAGENTA}%-18s${RESET} │ %11s ns │ %11s ns │ %11s ns │ %11s ns │\n" \
        "netbookmain (base)" "$bm_avg" "$bm_min" "$bm_max" "$bm_last"
    printf "│ ${CYAN}%-18s${RESET} │ %11s ns │ %11s ns │ %11s ns │ %11s ns │\n" \
        "netbook (cand)" "$cd_avg" "$cd_min" "$cd_max" "$cd_last"
    printf '%*s\n' "$W" '' | tr ' ' '═'
    echo
    printf "  ${BOLD}Delta (avg):${RESET}  ${colour}${sign}%s ns${RESET}     " "$delta"
    printf "${BOLD}Change:${RESET}  ${colour}${sign}%s%%${RESET}\n" "$pct"
    echo
    printf "  ${DIM}Green delta = candidate is faster · Red = slower${RESET}\n"
    printf "  ${DIM}All values in nanoseconds. Press Ctrl-C to stop.${RESET}\n\n"
}

# ── Cleanup on exit ───────────────────────────
trap 'echo -e "\n${DIM}Benchmark stopped.${RESET}"; exit 0' INT TERM

# ── Verify binaries exist ─────────────────────
for b in netbook netbookmain; do
    [[ -x "./${b}" ]] || { echo "ERROR: ./${b} not found or not executable." >&2; exit 1; }
done

# ── Main loop ─────────────────────────────────
run_count=0

while true; do
    if (( run_count % 2 == 0 )); then
        run_binary "netbookmain"
        run_binary "netbook"
    else
        run_binary "netbook"
        run_binary "netbookmain"
    fi

    run_count=$(( run_count + 1 ))
    draw_table "$run_count"
done