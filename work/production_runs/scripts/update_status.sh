#!/usr/bin/env bash
set -euo pipefail

ROOT="/home/bkoh/pgalf/work/production_runs"
LOCK="$ROOT/STATUS.lock"
TOTAL_EXPECTED=275

(
  flock -x 200
  TMP=$(mktemp)
  {
    echo "# STATUS.md"
    echo
    echo "Updated: $(date '+%Y-%m-%d %H:%M:%S %Z (%z)')"
    echo ""
    echo "| Variant | Completed | Passed | Failed | Pending | Manifest |"
    echo "|---|---:|---:|---:|---:|---|"

    for variant in zoom zoom2 zoom3 zoom4; do
      status_dir="$ROOT/$variant/runs/snapshot_status"
      mapfile -t status_files < <(find "$status_dir" -maxdepth 1 -type f -name 'snap_*.status' 2>/dev/null | sort)
      completed=${#status_files[@]}
      passed=0
      failed=0
      for sf in "${status_files[@]}"; do
        state=$(awk -F= '/^STATE=/{print $2}' "$sf" | tail -n1)
        if [[ "$state" == "PASS" ]]; then
          passed=$((passed+1))
        elif [[ "$state" == "FAIL" ]]; then
          failed=$((failed+1))
        fi
      done
      pending=$((TOTAL_EXPECTED-completed))
      if (( pending < 0 )); then pending=0; fi
      echo "| $variant | $completed | $passed | $failed | $pending | $variant/PRODUCTION_RUN_MANIFEST.md |"
    done

    echo
    echo "## Failure Log Pointers"
    any_fail=0
    for variant in zoom zoom2 zoom3 zoom4; do
      status_dir="$ROOT/$variant/runs/snapshot_status"
      mapfile -t fail_files < <(grep -l '^STATE=FAIL' "$status_dir"/snap_*.status 2>/dev/null | sort || true)
      if ((${#fail_files[@]} > 0)); then
        any_fail=1
        echo
        echo "### $variant"
        for sf in "${fail_files[@]}"; do
          snap=$(awk -F= '/^SNAPSHOT=/{print $2}' "$sf" | tail -n1)
          job=$(awk -F= '/^JOB=/{print $2}' "$sf" | tail -n1)
          err=$(awk -F= '/^LOG_ERR=/{print $2}' "$sf" | tail -n1)
          stage=$(awk -F= '/^FAILED_STAGE=/{print $2}' "$sf" | tail -n1)
          echo "- snap_${snap} job=${job} stage=${stage} err=${err}"
        done
      fi
    done
    if (( any_fail == 0 )); then
      echo "- No failures recorded."
    fi

    echo
    echo "## Monitoring"
    echo "- squeue -u \$USER"
    echo "- tail -f /home/bkoh/pgalf/work/production_runs/<variant>/logs/snap_<idx>.out"
    echo "- tail -f /home/bkoh/pgalf/work/production_runs/<variant>/logs/snap_<idx>.err"
  } > "$TMP"
  mv "$TMP" "$ROOT/STATUS.md"
) 200>"$LOCK"
