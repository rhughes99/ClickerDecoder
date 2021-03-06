/*	Clicker Decoder Controller
	Tuned for Philips / Magnavox TV remote
    (Smaller controller only for TV)
	Shared memory example
*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>

#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#define PRU_ADDR		0x4A300000		// Start of PRU memory Page 184 am335x TRM
#define PRU_LEN			0x80000			// Length of PRU memory
#define PRU0_DRAM		0x00000			// Offset to DRAM
#define PRU1_DRAM		0x02000
#define PRU_SHAREDMEM	0x10000			// Offset to shared memory

unsigned int *pru0DRAM_32int_ptr;		// Points to the start of PRU 0 usable RAM
unsigned int *pru1DRAM_32int_ptr;		// Points to the start of PRU 1 usable RAM
unsigned int *prusharedMem_32int_ptr;	// Points to the start of shared memory

void myShutdown(int sig);
void ClearDisplay();
void SetUpX();
void SetUp8Square();
void SetUp6Square();
void SetUp4Square();
void SetUp2Square();

unsigned char running;
unsigned char displayPresent = 1;    // assume display on I2C bus
unsigned char dispBuffer[16];

//____________________
int main(int argc, char *argv[])
{
	unsigned int *pru;		// Points to start of PRU memory
	unsigned int theCmd, lastCmd;
	unsigned int *pruDRAM_32int_ptr;
	int	i, fd, file;
    char cmdBuffer[1];

	fd = open ("/dev/mem", O_RDWR | O_SYNC);
	if (fd == -1)
	{
		printf ("*** ERROR: could not open /dev/mem.\n");
		return EXIT_FAILURE;
	}
	pru = mmap (0, PRU_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, PRU_ADDR);
	if (pru == MAP_FAILED)
	{
		printf ("*** ERROR: could not map memory.\n");
		return EXIT_FAILURE;
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

	// I2C Setup
    if ((file = open("/dev/i2c-2", O_RDWR)) < 0)
    {
        perror("*** Failed to open I2C bus dev file\n");
        return 1;
    }

    if (ioctl(file, I2C_SLAVE, 0x71) < 0)
    {
        perror("*** Failed to connect to I2C device\n");
        return 1;
    }

    // Internal system clock enable
    cmdBuffer[0] = 0x21;
    if (write(file, cmdBuffer, 1) != 1)
    {
        perror("*** Failed to send System Setup cmd: Device not present\n");
        displayPresent = 0;
    }

    if (displayPresent)
    {
        // Display on
        cmdBuffer[0] = 0x81;        // no blink
        write(file, cmdBuffer, 1);

        // ROW/INT output pin set
        cmdBuffer[0] = 0xA0;
        write(file, cmdBuffer, 1);

        // Dimming = default

        // Blinking = default

        // Clear display
        ClearDisplay();
        write(file, dispBuffer, 16);
     }

	(void) signal(SIGINT, myShutdown);

	running = 1;
	lastCmd = 0;
	printf("\nClickerDecoder running...\n");
	if (displayPresent)
		printf("   (Display present)\n");
	else
		printf("   (Display not present)\n");

	do
	{
		sleep(0.1);			// 1 command takes ~24 ms

		theCmd = pruDRAM_32int_ptr[0];
		if (theCmd != lastCmd)
		{
			switch (theCmd)
			{
                case 11184981:                          // 0xAAAB55
                case 19573589:                          // 0x12AAB55
                    printf("STATUS/EXIT\n");
                    if (displayPresent)
                    {
                        ClearDisplay();
                        write(file, dispBuffer, 16);
                    }
                    break;

				case 11186859:                          // 0xAAB2AB
                case 19575467:                          // 0x12AB2AB
                    printf("CH +\n");
                    break;

				case 11184971:
				case 19573579:
					printf("--- POWER ---\n");
					if (displayPresent)
					{
						SetUpX();
				        write(file, dispBuffer, 16);
					}
					break;

				case 11185325:                          // 0xAAACAD
                case 19573933:                          // 0x12AACAD
                    printf("VOL -\n");
                    break;

				case 11187027:                          // 0xAAB353
                case 19575635:                          // 0x12AB353
                    printf("MENU\n");
                    break;

				case 11185323:                          // 0xAAACAB
                case 19573931:                          // 0x12AACAB
                    printf("VOL +\n");
                    break;

                case 11184973:                          // 0xAAAB4D
				case 19573581:                          // 0x12AAB4D
                    printf("MUTE\n");
                    break;

                case 11186861:                          // 0xAAB2AD
				case 19575469:                          // 0x12AB2AD
                    printf("CH -\n");
                    break;

                case 11186867:                          // 0xAAB2B3
				case 19575475:                          // 0x12AB2B3
                    printf("A/CH\n");
                    break;

                case 11184813:                          // 0xAAAAAD
				case 19573421:                          // 0x12AAAAD
                    printf("1\n");
                    break;

                case 11184819:                          // 0xAAAAB3
				case 19573427:                          // 0x12AAAB3
                    printf("2\n");
					if (displayPresent)
					{
						SetUp2Square();
				        write(file, dispBuffer, 16);
					}
                    break;

                case 11184821:                          // 0xAAAAB5
				case 19573429:                          // 0x12AAAB5
                    printf("3\n");
                    break;

                case 11184843:                          // 0xAAAACB
				case 19573451:                          // 0x12AAACB
                    printf("4\n");
					if (displayPresent)
					{
						SetUp4Square();
				        write(file, dispBuffer, 16);
					}
                    break;

                case 11184845:                          // 0xAAAACD
				case 19573453:                          // 0x12AAACD
                    printf("5\n");
                    break;

                case 11184851:                          // 0xAAAAD3
				case 19573459:                          // 0x12AAAD3
                    printf("6\n");
					if (displayPresent)
					{
						SetUp6Square();
				        write(file, dispBuffer, 16);
					}
                    break;

                case 11184853:                          // 0xAAAAD5
				case 19573461:                          // 0x12AAAD5
                    printf("7\n");
                    break;

                case 11184939:                          // 0xAAAB2B
				case 19573547:                          // 0x12AAB2B
                    printf("8\n");
					if (displayPresent)
					{
						SetUp8Square();
				        write(file, dispBuffer, 16);
					}
                    break;

                case 11184941:                          // 0xAAAB2D
				case 19573549:                          // 0x12AAB2D
                    printf("9\n");
                    break;

                case 11184811:                          // 0xAAAAAB
				case 19573419:                          // 0x12AAAAB
                    printf("0\n");
                    if (displayPresent)
                    {
                        ClearDisplay();
                        write(file, dispBuffer, 16);
                    }
                    break;

                case 11185491:                          // 0xAAAD53
				case 19574099:                          // 0x12AAD53
                    printf("CC\n");
                    break;

                case 11186899:                          // 0xAAB2D3
				case 19575507:                          // 0x12AB2D3
                    printf("SLEEP\n");
                    break;

/* These are VCR commands used by TV/VCR remote
				case 11324235:							// 0x0ACCB4B
				case 19712843:							// 0x12CCB4B
					printf("--- VCR POWER ---\n");
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
*/

				default:
					printf("Unknown cmd: %d (0x%X)\n", theCmd, theCmd);
			}
			lastCmd = theCmd;
		}
	} while (running);

	printf ("---Shutting down...\n");

	if(munmap(pru, PRU_LEN))
		printf("*** ERROR: munmap failed at Shutdown\n");

	return EXIT_SUCCESS;
}

//____________________
void myShutdown(int sig)
{
	// ctrl-c
	running = 0;
	(void) signal(SIGINT, SIG_DFL);		// reset signal handling of SIGINT
}

//____________________
void ClearDisplay()
{
    // Clear dispBuffer; note that every other byte is used
    int i;
    for (i=0; i<16; i++)
        dispBuffer[i] = 0;
}

//____________________
void SetUpX()
{
    dispBuffer[0x1] = 0xC0;
    dispBuffer[0x3] = 0x21;
    dispBuffer[0x5] = 0x12;
    dispBuffer[0x7] = 0x0C;
    dispBuffer[0x9] = dispBuffer[0x7];
    dispBuffer[0xB] = dispBuffer[0x5];
    dispBuffer[0xD] = dispBuffer[0x3];
    dispBuffer[0xF] = dispBuffer[0x1];
}

//____________________
void SetUp8Square()
{
    dispBuffer[0x1] = 0xFF;
    dispBuffer[0x3] = 0xC0;
    dispBuffer[0x5] = 0xC0;
    dispBuffer[0x7] = 0xC0;
    dispBuffer[0x9] = 0xC0;
    dispBuffer[0xB] = 0xC0;
    dispBuffer[0xD] = 0xC0;
    dispBuffer[0xF] = 0xFF;
}

//____________________
void SetUp6Square()
{
    dispBuffer[0x1] = 0x00;
    dispBuffer[0x3] = 0x3F;
    dispBuffer[0x5] = 0x21;
    dispBuffer[0x7] = 0x21;
    dispBuffer[0x9] = 0x21;
    dispBuffer[0xB] = 0x21;
    dispBuffer[0xD] = 0x3F;
    dispBuffer[0xF] = 0x00;
}

//____________________
void SetUp4Square()
{
    dispBuffer[0x1] = 0x00;
    dispBuffer[0x3] = 0x00;
    dispBuffer[0x5] = 0x1E;
    dispBuffer[0x7] = 0x12;
    dispBuffer[0x9] = 0x12;
    dispBuffer[0xB] = 0x1E;
    dispBuffer[0xD] = 0x00;
    dispBuffer[0xF] = 0x00;
}

//____________________
void SetUp2Square()
{
    dispBuffer[0x1] = 0x00;
    dispBuffer[0x3] = 0x00;
    dispBuffer[0x5] = 0x00;
    dispBuffer[0x7] = 0x0C;
    dispBuffer[0x9] = 0x0C;
    dispBuffer[0xB] = 0x00;
    dispBuffer[0xD] = 0x00;
    dispBuffer[0xF] = 0x00;
}
