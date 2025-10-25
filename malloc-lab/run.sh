#!/usr/bin/env bash
set -euo pipefail

# 기본 설정
TRACES=("$@")
if [ ${#TRACES[@]} -eq 0 ]; then
  # 인자를 안 주면 기본 제공 트레이스만
  TRACES=("short1-bal.rep" "short2-bal.rep")
fi

# 결과 저장 폴더
STAMP="$(date +%Y%m%d_%H%M%S)"
OUTDIR="results/${STAMP}"
mkdir -p "${OUTDIR}"

# 빌드 (AFS handin cp 라인 때문에 make 실패하면, 그 라인 주석 처리 권장)
echo "[build] make"
make -j || { echo "⚠️  make 실패. Makefile의 AFS handin cp 라인을 주석 처리하세요."; exit 1; }

# 드라이버 존재 확인
if [ ! -x ./mdriver ]; then
  echo "❌ mdriver 실행파일이 없습니다. make 결과를 확인하세요."
  exit 1
fi

LOG="${OUTDIR}/run.log"
SUMMARY="${OUTDIR}/summary.csv"
echo "trace,valid,util,ops,secs,kops" > "${SUMMARY}"

echo "[run] traces: ${TRACES[*]}"
{
  echo "===== RUN @ $(date) ====="
  ./mdriver -h | head -n 3
  echo
} > "${LOG}"

for t in "${TRACES[@]}"; do
  echo "[run] ./mdriver -V -f ${t}"
  echo ">>> TRACE ${t}" >> "${LOG}"
  ./mdriver -V -f "${t}" >> "${LOG}" 2>&1

  # per-trace 표에서 숫자 라인 파싱 (형식: ' 0    yes   28%   12  0.000000 60000')
  awk -v trace="${t}" '
    /^[[:space:]]*[0-9]+[[:space:]]+/ {
      # 컬럼: trace valid util ops secs Kops
      # util에서 % 제거
      tr=$1; vd=$2; ut=$3; gsub("%","",ut); op=$4; sc=$5; kp=$6;
      print trace","vd","ut","op","sc","kp
    }
  ' "${LOG}" | tail -n 1 >> "${SUMMARY}"
done

echo
echo "✅ Done."
echo "📄 Log:      ${LOG}"
echo "📊 Summary:  ${SUMMARY}"
column -t -s, "${SUMMARY}" || true