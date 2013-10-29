#include <stdio.h>
#include <malloc.h>

struct Ration2m
{
  float ratio;
  int n, m;
};

#define IPU_LUT_LEN (32)
struct host_ipu_table
{
	int cnt;
	struct Ration2m ipu_ratio_table[IPU_LUT_LEN*IPU_LUT_LEN];
};

#if 0
int main(int argc, char *argv[])
{
	unsigned int i, j, cnt;
	float diff;
	struct Ration2m *ipu_ratio_table;


	ipu_ratio_table = (struct Ration2m *)malloc(sizeof(struct Ration2m) * IPU_LUT_LEN*IPU_LUT_LEN);

	if (ipu_ratio_table == 0) {
		printf("too small memory!\n");
		return (-1);
	}
	//      get_ipu_ratio_table( (void **)&ipu_ratio_table, (IPU_LUT_LEN) * (IPU_LUT_LEN)*sizeof(struct Ration2m) );
	// orig table, first calculate
	for (i = 1; i <= (IPU_LUT_LEN); i++) {
		for (j = 1; j <= (IPU_LUT_LEN); j++) {
			ipu_ratio_table[(i - 1) * IPU_LUT_LEN + j - 1].ratio = i / (float)j;
			ipu_ratio_table[(i - 1) * IPU_LUT_LEN + j - 1].n = i;
			ipu_ratio_table[(i - 1) * IPU_LUT_LEN + j - 1].m = j;
		}
	}
	// Eliminate the ratio greater than 1:2
	for (i = 0; i < (IPU_LUT_LEN) * (IPU_LUT_LEN); i++) {
		if (ipu_ratio_table[i].ratio < 0.031249) {
			ipu_ratio_table[i].n = ipu_ratio_table[i].m = -1;
		}
	}
	// eliminate the same ratio
	for (i = 0; i < (IPU_LUT_LEN) * (IPU_LUT_LEN); i++) {
		for (j = i + 1; j < (IPU_LUT_LEN) * (IPU_LUT_LEN); j++) {
			diff =
			    ipu_ratio_table[i].ratio - ipu_ratio_table[j].ratio;
			if (diff > -0.001 && diff < 0.001) {
				ipu_ratio_table[j].n = -1;
				ipu_ratio_table[j].m = -1;
			}
		}
	}

	// reorder ipu_ratio_table
	cnt = 0;
	for (i = 0; i < (IPU_LUT_LEN) * (IPU_LUT_LEN); i++) {
		if (ipu_ratio_table[i].n != -1) {
			if (cnt != i) {
				ipu_ratio_table[cnt] = ipu_ratio_table[i];
			}
			cnt++;
		}
	}
	printf("#include "jz_ipu.h"\n");
	printf("struct host_ipu_table hostiputable = {");
	printf("{%d},{\n\t",cnt);	
	int n = 0;
	for (i = 1; i <= (IPU_LUT_LEN); i++) {
		for (j = 1; j <= (IPU_LUT_LEN); j++) {
			printf("{%06f, %2d, %2d}",
			       ipu_ratio_table[(i - 1) * IPU_LUT_LEN + j - 1].ratio,
			       ipu_ratio_table[(i - 1) * IPU_LUT_LEN + j - 1].n,
			       ipu_ratio_table[(i - 1) * IPU_LUT_LEN + j - 1].m);
			n++;
			if((n != IPU_LUT_LEN * IPU_LUT_LEN))
			{
				if((n % 4 == 0))
				{
					printf(",\n\t");
				}else
					printf(",");
			}else
				printf("\n\t");
		}
	}
	printf("}};");
	return (0);
}
#endif

