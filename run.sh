
EXP_NAME="convert"
O_DIR=logs/${EXP_NAME}
mkdir -p ${O_DIR}

DATA_DIR=/data1/mix
VTUNE=./vtune.sh  # ${O_DIR}/${PREFIX}
PCM=/home/yuang/Programs/pcm/pcm-core.x
LIWID=${measure} 
PERF=./perf.sh
measure=${VTUNE}

THREAD=20
SIZE=256
ROUND=1

for nRow in 21 22 23 24 25 ; do #tr wl weibo pld kr24 rmat23_631
    for APP in pr; do
        for Deg in 16 32 64; do
        PREFIX=${APP}.rmat_${nRow}_${Deg}
        echo ${PREFIX}
        ./app/${APP} -d /data/rmat/rmat-6211/rmat_${nRow}_${Deg}.csr -o /data/rmat/rmat-6211/rmat_${nRow}_${Deg}.mix  -i 100 -t ${THREAD} -r ${ROUND} -s ${SIZE} #> ${O_DIR}/${PREFIX}.log 2>&1
        done 
    done
done

# likwid-perfctr -f -g MEM 
# likwid-perfctr -f -g L2CACHE
# ./vtune.sh ${O_DIR}/${PREFIX} 