#!/bin/bash

LOG_DIR="./valgrind_logs"
SUMMARY_FILE="$LOG_DIR/valgrind_summary.log"

mkdir -p "$LOG_DIR"
: > "$SUMMARY_FILE"  # 清空 summary 文件

echo "==========================================================================================================================" | tee -a "$SUMMARY_FILE"
echo "===================== 🧠 VALGRIND MEMORY TEST BEGIN ======================" | tee -a "$SUMMARY_FILE"
echo "Working Directory  : $(pwd)" | tee -a "$SUMMARY_FILE"
echo "Start Time         : $(date)" | tee -a "$SUMMARY_FILE"
echo "==========================================================================================================================" | tee -a "$SUMMARY_FILE"

for exe in *; do
    if [[ -x "$exe" && ! -d "$exe" && "$exe" != "valgrind_in_bin.sh" ]]; then
        LOG_FILE="$LOG_DIR/valgrind_${exe}.log"

        echo -e "\n\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" | tee -a "$SUMMARY_FILE"
        echo "==================== 🚀 TEST START: $exe ====================" | tee -a "$SUMMARY_FILE"
        echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" | tee -a "$SUMMARY_FILE"

        valgrind \
          --leak-check=full \
          --show-leak-kinds=all \
          --track-origins=yes \
          --verbose \
          --log-file="$LOG_FILE" \
          --error-exitcode=1 \
          --num-callers=40 \
          "./$exe"

        if [[ $? -ne 0 ]]; then
            echo "❌ Memory issues detected in $exe (see $LOG_FILE)" | tee -a "$SUMMARY_FILE"
        else
            echo "✅ No leaks detected in $exe" | tee -a "$SUMMARY_FILE"
        fi

        echo "📄 Output log saved: $LOG_FILE" | tee -a "$SUMMARY_FILE"
        echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" | tee -a "$SUMMARY_FILE"
        echo "==================== 🛑 TEST END: $exe ======================" | tee -a "$SUMMARY_FILE"
        echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" | tee -a "$SUMMARY_FILE"
    fi
done

echo -e "\n==========================================================================================================================" | tee -a "$SUMMARY_FILE"
echo "====================== ✅ VALGRIND MEMORY TEST END =======================" | tee -a "$SUMMARY_FILE"
echo "End Time           : $(date)" | tee -a "$SUMMARY_FILE"
echo "==========================================================================================================================" | tee -a "$SUMMARY_FILE"
