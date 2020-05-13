#define _GNU_SOURCE
#include <stdio.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include <stdint.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <time.h> 
#include "../cacheutils.h"
#define SUM_KEY 22
const int length = 4*256*1024;//8MB shm

void cache_flush(){
	const int flush = 5*1024*1024; // Allocate 20M. Set much larger then L2
    char *c = (char *)malloc(flush);
    for (int i = 0; i < 0x7; ++i)
    {
    	for (int j = 0; j < flush; j++)
		{
			c[j] = j/2;
		}	
    }
}
int main(int argc, char const *argv[])
{
	// SET PROC TO CPU0
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(0, &mask);
	int result = sched_setaffinity(0, sizeof(mask), &mask);

	// INPUT ARGS & FLAGS
	int threshold = atoi(argv[1]);
	int attack_flag = atoi(argv[2]);
	// int cf_flag = atoi(argv[3]);

	//SHARED MEMORY INIT
	int shm_id = shmget(SUM_KEY, length*sizeof(size_t), 0777 | IPC_CREAT);
	size_t* shm = shmat(shm_id, 0, 0);
	*(long long int*)shm = 0;
	for (int i = 0; i < length; i++){
		shm[i] = (long long int)i;
	}

	for (int iter = 0; iter < 30; iter++)
	{
		printf("(%d) ",iter );
		int hit_cnt[64]={0};
		
		for(int line = 0; line < 64; line++){
			
			for (int page = 0; page < length/512; page++)
			{
				int flag=0;
				int pid, status;

				for (int i = 0; i < length/8; i++){
					flush(&shm[i]);
				}

				if(attack_flag) //EXEC ATTACK
				{
					int temper, l1, l2, l3;
					/*In this block we can change l1, l2, and l3 to reproduce all the Cases described in the paper.*/
					l1 = page*512 + 8*0;
					// l2 = page*512 + 8*63+5;					
					// l3 = page*512 + 8*63+2;
					l2 = page*512 + 8*1;
					l3 = page*512 + 8*2;
					temper = shm[l1];
					temper = shm[l2];
					temper = shm[l3];

				}
				// victim access
				int tr = page*512 + 8*20;
				int temp = shm[tr]; //TRIGGER PREFETCH

				uint64_t a, d, e, f;
				size_t temp12;
				int temp_cnt = 512*(page)+line*8;
				asm volatile("mfence");
				asm volatile("rdtsc": "=a"(a), "=d"(d));
				temp12 = shm[temp_cnt];
				asm volatile("mfence");
				asm volatile("rdtsc": "=a"(e), "=d"(f));

				if((e-a)<threshold)
					hit_cnt[line]++;
			}
		}
		for (int i = 0; i < 64; ++i)
		{
			printf("%d ",hit_cnt[i] );
		}
		printf("\n");
	}
	shmctl(shm_id, IPC_RMID, NULL);
	shmdt(shm);
	return 0;
}
/*
▶ taskset 0x1 ./new_rdtsc_no_avg 110 1 0

(0) 2048 1373 1529 1458 1487 1172 1440 0 0 0 0 0 0 0 0 0 0 0 0 0 2047 1998 2040 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(1) 2047 1497 1576 1491 1651 1593 1693 0 0 0 0 0 0 0 0 0 0 0 0 0 2047 2041 2041 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(2) 2047 1426 1543 1390 1320 1477 1485 0 0 0 0 0 0 0 0 0 0 0 0 0 2047 2045 2040 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(3) 2048 1637 1610 1449 1500 1557 1536 0 0 0 0 1 0 0 0 0 0 0 0 0 2048 2042 2044 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(4) 2047 1159 1066 1192 1453 1340 1465 0 0 0 0 0 0 0 0 0 0 0 0 0 2048 2047 2048 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(5) 2046 1242 1327 1334 1328 1362 1343 0 1 0 0 0 0 0 0 0 0 0 0 0 2047 2047 2047 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(6) 2048 1375 1289 1313 1295 1456 1199 0 0 0 0 0 0 0 0 0 0 0 0 0 2048 2045 2046 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(7) 2046 1424 1302 1200 1304 1397 1334 0 0 0 0 0 0 0 0 0 0 0 0 0 2047 2047 2043 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(8) 2047 1369 1353 1440 1325 1361 1269 0 0 0 0 0 0 0 0 0 0 0 0 0 2048 2046 2045 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(9) 2048 1405 1386 1475 1402 1539 1428 0 0 0 0 0 0 0 0 0 0 0 0 0 2048 2044 2045 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(10) 2046 1544 1314 1301 1366 1434 1384 0 0 0 0 0 0 0 0 0 0 0 0 0 2046 2044 2042 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(11) 2047 1346 1548 1386 1373 1417 1327 0 0 0 0 0 0 0 0 0 0 0 0 0 2047 2044 2042 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(12) 2048 1238 1271 1340 1260 1214 1334 0 0 0 0 0 0 0 0 0 0 0 0 0 2048 2046 2048 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(13) 2048 1300 1429 1467 1502 1356 1274 0 0 0 0 0 0 0 0 0 0 0 0 0 2047 2042 2043 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(14) 2048 1309 1347 1315 1325 1331 1302 0 0 0 0 0 0 0 0 0 0 0 0 0 2047 2043 2045 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(15) 2046 1289 1494 1437 1512 1492 1437 0 0 0 0 0 0 0 0 0 0 0 0 0 2048 2047 2047 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(16) 2048 1370 1369 1266 1492 1411 1414 0 0 0 0 0 0 0 0 0 0 0 0 0 2046 2047 2046 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(17) 2048 1457 1408 1360 1307 1541 1555 0 0 0 0 0 0 0 0 0 0 0 0 0 2048 2044 2046 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(18) 2047 1540 1428 1477 1418 1524 1383 0 0 0 0 0 0 0 0 0 0 0 0 0 2047 2045 2045 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(19) 2047 1377 1338 1412 1369 1347 1341 0 0 0 0 0 0 0 0 0 0 0 0 0 2047 2047 2046 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(20) 2047 1373 1428 1498 1596 1509 1447 0 0 0 0 0 0 0 0 0 0 0 0 0 2048 2041 2047 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(21) 2048 1507 1428 1485 1399 1358 1411 1 0 0 0 0 0 0 0 0 0 0 0 0 2047 2047 2045 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(22) 2046 1386 1424 1593 1527 1571 1459 0 0 0 0 0 0 0 0 0 0 0 0 0 2048 2048 2046 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(23) 2048 1672 1611 1587 1455 1431 1414 0 0 0 0 0 0 0 0 0 0 0 0 0 2047 2041 2046 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(24) 2046 1516 1461 1328 1367 1307 1416 0 0 0 0 0 0 0 0 0 0 0 0 0 2048 2044 2048 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(25) 2046 1364 1474 1384 1441 1399 1521 0 0 0 0 0 0 0 0 0 0 0 0 0 2047 2046 2045 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(26) 2048 1376 1456 1371 1404 1355 1439 0 0 0 0 0 0 0 0 0 0 0 0 0 2047 2043 2041 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(27) 2046 1416 1549 1408 1536 1611 1545 0 0 0 0 0 0 0 0 0 0 0 0 0 2048 2046 2048 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(28) 2048 1309 1356 1486 1403 1431 1519 0 0 0 0 0 0 0 0 0 0 0 0 0 2047 2043 2048 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
(29) 2048 1434 1449 1357 1481 1354 1405 0 0 0 0 0 0 0 0 0 0 0 0 0 2046 2044 2045 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
*/