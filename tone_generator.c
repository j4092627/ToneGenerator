#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include "portaudio.h"

#define PI		3.14159265358979323846
#define BUF_LEN     480 // 10 ms buffer in Sound data structure
#define FRAMES_PER_BUFFER 512   //buffer in portaudio i/o buffer

typedef struct {
           float *tone;
           unsigned int num_samp;
           unsigned int next_samp;
           unsigned int count;
     } soundData;
     soundData data;

/* Callback function protoype */
static int paCallback( const void *inputBuffer, void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData );

int main(int argc, char *argv[])
{
	double f0 = 440; // frequency of tone to play out
    double fs = 48000; // sampling rate of play out
    int level_dBFS = -24; // level of play out, dBFS

    char input[10];
    int n;

	//initial value
	data.num_samp = fs;
	data.next_samp = 0;
	data.count = 0;

	//
	PaStream *stream;
    PaError err;
    PaStreamParameters outputParams;
    PaStreamParameters inputParams;

	//parse command line
	for(int i = 1; i<argc; i++)
	{
		//char input[10];
		memset(&input[0], 0, sizeof(input));

		if(argv[i][0] == '-')
		{
			if(argv[i][1] == 'a')
			{
				for(int j = 0; j<strlen(argv[i])-2; j++)
				{
					input[j] = argv[i][j+2];
				}
				level_dBFS = atof(input);
				//printf("db: %d\n",level_dBFS);
			}
			else if(argv[i][1] == 'f')
			{
				for(int j = 0; j<strlen(argv[i])-2; j++)
				{
					input[j] = argv[i][j+2];
				}
				f0 = atof(input);
				//printf("frequency: %f\n",f0);
			}
			else if(argv[i][1] == 's')
			{
				for(int j = 0; j<strlen(argv[i])-2; j++)
				{
					input[j] = argv[i][j+2];
				}
				fs = atof(input);
				//printf("sample rate: %f\n",fs);
			}
			else
			{
				printf("Command line error, please input as tone_generator [–f freq_Hz] [–a level_dBFS] [-s sample_rate_fs]\n");
				return 0;
			}
		}
		else
			{
				printf("Command line error, please input as tone_generator [–f freq_Hz] [–a level_dBFS] [-s sample_rate_fs]\n");
				return 0;
			}
	}
	n = round(fs/f0);
    f0 = fs/n;
    data.num_samp = n;

	printf("level_dBFS: %d\n",level_dBFS);
	printf("frequency: %f\n",f0);
	printf("sample rate: %f\n",fs);

	if (n > BUF_LEN) {
        fprintf(stderr, "ERROR: wavelength is too long for tone buffer\n");
        return 11;
    }

	double level = pow(10, level_dBFS/20.0);
	printf("level:%f\n",level);
	//float level = 0.5;
	if((data.tone = (float*)malloc(data.num_samp *sizeof(float))) == NULL)
		{
			printf("There's not enough space for storing data\n");
		}
	for(int i=0; i<data.num_samp; i++)
	{
		data.tone[i] = (float)level * sin(2*PI*i*f0/fs);
		
	}

	/* Initializing PortAudio */
    err = Pa_Initialize();
    if (err != paNoError) {
        printf("PortAudio error: %s\n", Pa_GetErrorText(err));
        printf("\nExiting.\n");
        exit(1);
    }

    /* Input stream parameters */
    inputParams.device = Pa_GetDefaultInputDevice();
    inputParams.channelCount = 1;
    inputParams.sampleFormat = paFloat32;
    inputParams.suggestedLatency =
        Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = NULL;

    /* Ouput stream parameters */
    outputParams.device = Pa_GetDefaultOutputDevice();
    outputParams.channelCount = 2;
    outputParams.sampleFormat = paFloat32;
    outputParams.suggestedLatency =
        Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = NULL;

    /* Open audio stream */
    err = Pa_OpenStream(&stream,
        &inputParams, /* no input */
        &outputParams,
        fs, FRAMES_PER_BUFFER,
        paNoFlag, /* flags */
        paCallback,
        &data);

    if (err != paNoError) {
        printf("PortAudio error: open stream: %s\n", Pa_GetErrorText(err));
        printf("\nExiting.\n");
        exit(1);
    }

    /* Start audio stream */
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        printf(  "PortAudio error: start stream: %s\n", Pa_GetErrorText(err));
        printf("\nExiting.\n");
        exit(1);
    }
	
    printf("Starting playout\n");
	while (data.count < fs * 5) 
	{
        sleep(1);  // reduces consumption of CPU resources
	}
	printf("Finished playout\n");

    /* Stop stream */
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        printf(  "PortAudio error: stop stream: %s\n", Pa_GetErrorText(err));
        printf("\nExiting.\n");
        exit(1);
    }

    /* Close stream */
    err = Pa_CloseStream(stream);
    if (err != paNoError) {
        printf(  "PortAudio error: close stream: %s\n", Pa_GetErrorText(err));
        printf("\nExiting.\n");
        exit(1);
    }

    /* Terminate PortAudio */
    err = Pa_Terminate();
    if (err != paNoError) {
        printf("PortAudio error: terminate: %s\n", Pa_GetErrorText(err));
        printf("\nExiting.\n");
        exit(1);
    }

    return 0;
}

static int paCallback(const void *inputBuffer, void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData)
{
    /* Cast data passed through stream to our structure. */
    soundData *data = (soundData *)userData;
    float *out = (float *)outputBuffer;
    unsigned int i, j;

    /* Fill buffer from tone data */
    j = data->next_samp;
    for (i = 0; i < framesPerBuffer; i++) {
        /* check if j should wrap */
        if (j >= data->num_samp) {
            j = 0;
        }
        /* same signal to left and right channels
         * left and right channels are interleaved in buffer out[]
         */
        out[2*i] = data->tone[j];  /* left */
        out[2*i+1] = data->tone[j];  /* right */
        j++; /* increment pointer to tone[] */
        data->count++;
    }

    data->next_samp = j;
    return 0;
}