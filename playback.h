
#ifndef __playback__
#define __playback__
#include <stdio.h>  
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  
#include <fcntl.h> 
#include <alsa/asoundlib.h>

typedef unsigned char  uint8_t;  
typedef unsigned int   uint32_t;  

#define CHANNELS         (2)  
#define SAMPLE_RATE      (48000)  
#define SAMPLE_LENGTH    (16)

typedef struct SNDPCMContainer
{
	snd_pcm_t         *handle; /*��PCM�豸�ľ��ָ��*/
	snd_pcm_uframes_t buffer_size;/*��frameΪ��λ*/
	snd_pcm_uframes_t period_size;/*��frameΪ��λ*/
	size_t 			  period_byte;/*һ�δ������ݵ��ֽ���*/
	snd_pcm_format_t  format;	  /*����λ��*/
	uint32_t		  channels;
	size_t 			  bits_per_sample;/*һ�β���������bit��Ŀ*/  
    size_t            bits_per_frame;/*һ֡���ݵ�bit��Ŀ*/  
    uint8_t           *data_buf;/*������ʱ�洢����/д���PCM����*/  
}SNDPCMContainer_t;


int SNDPCM_SetParams(SNDPCMContainer_t *);
int SNDPCM_Record_And_Play(SNDPCMContainer_t *,SNDPCMContainer_t *);
u_int32_t SND_ReadPcm(SNDPCMContainer_t *, size_t );
u_int32_t SND_WritePcm(SNDPCMContainer_t *, size_t );








































































#endif
