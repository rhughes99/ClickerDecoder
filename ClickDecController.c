/*	Clicker Decoder Controller
	Tuned for Magnavox VCR remote: VCR & TV
	Shared memory example
*/
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>

#define PRU_ADDR		0x4A300000		// Start of PRU memory Page 184 am335x TRM
#define PRU_LEN			0x80000			// Length of PRU memory
#define PRU0_DRAM		0x00000			// Offset to DRAM
#define PRU1_DRAM		0x02000
#define PRU_SHAREDMEM	0x10000			// Offset to shared memory

unsigned int *pru0DRAM_32int_ptr;		// Points to the start of PRU 0 usable RAM
unsigned int *pru1DRAM_32int_ptr;		// Points to the start of PRU 1 usable RAM
unsigned int *prusharedMem_32int_ptr;	// Points to the start of shared memory

void myShutdown(int sig);

unsigned char running;

//____________________
int main(int argc, char *argv[])
{
	unsigned int *pru;		// Points to start of PRU memory
	unsigned int theCmd, lastCmd;
	unsigned int *pruDRAM_32int_ptr;
	int	i, fd;

	fd = open ("/dev/mem", O_RDWR | O_SYNC);
	if (fd == -1)
	{
		printf ("*** ERROR: could not open /dev/mem.\n\n");
		return 1;
	}
	pru = mmap (0, PRU_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, PRU_ADDR);
	if (pru == MAP_FAILED)
	{
		printf ("*** ERROR: could not map memory.\n\n");
		return 1;
	}
	close(fd);
//	printf ("Using /dev/mem.\n");

	// Set memory pointers
	pru0DRAM_32int_ptr =     pru + PRU0_DRAM/4 + 0x200/4;	// Points to 0x200 of PRU0 memory
	pru1DRAM_32int_ptr =     pru + PRU1_DRAM/4 + 0x200/4;	// Points to 0x200 of PRU1 memory
	prusharedMem_32int_ptr = pru + PRU_SHAREDMEM/4;			// Points to start of shared memory

	pruDRAM_32int_ptr = pru0DRAM_32int_ptr;

	for(i=0; i<8; i++)
	{
		if (pruDRAM_32int_ptr[i] != i)
			printf("*** Unexpected value read from PRU: %d %d\n", i, pruDRAM_32int_ptr[i]);
	}

	(void) signal(SIGINT, myShutdown);

	running = 1;
	lastCmd = 0;
	printf("\nClickerDecoder running...\n");
	do
	{
		sleep(0.1);			// 1 command takes ~24 ms

		theCmd = pruDRAM_32int_ptr[0];
		if (theCmd != lastCmd)
		{
			switch (theCmd)
			{
				case 19712843:							// 0x12CCB4B
				case 11324235:							// 0x0ACCB4B
					printf("--- VCR POWER ---\n");
					break;

				case 11184971:
				case 19573579:
					printf("--- TV POWER ---\n");
					break;

				case 11326803:							// 0x0ACD553
				case 19715411:							// 0x12CD553
					printf("VCR/TV\n");
					break;

				case 11326285:							// 0x0ACD34D
				case 19714893:							// 0x12CD34D
					printf("VCR: EJECT\n");
					break;

				case 11326669:							// 0x0ACD4CD
				case 19715277:							// 0x12CD4CD
					printf("VCR: PLAY\n");
					break;

				case 11326643:
				case 19715251:
					printf("VCR: REW\n");
					break;

				// And a whole lot more...

				default:
					printf("Unknown cmd: %d (0x%X)\n", theCmd, theCmd);
			}
			lastCmd = theCmd;
		}
	} while (running);

	printf ("---Shutting down...\n");

	if(munmap(pru, PRU_LEN))
		printf("*** munmap failed at Shutdown\n");

	else
		printf("munmap succeeded\n");

	return EXIT_SUCCESS;
}

//____________________
void myShutdown(int sig)
{
	// ctrl-c
	running = 0;
	(void) signal(SIGINT, SIG_DFL);		// reset signal handling of SIGINT
}
