

#include "playback.h"

int main(int argc, char *argv[])
{
	int ret = -1;
	char *dev_name = "default";
	SNDPCMContainer_t record;
	SNDPCMContainer_t playback;

	memset(&record, 0x0, sizeof(record));
	memset(&playback, 0x0, sizeof(record));

	ret = snd_pcm_open(&record.handle,dev_name,SND_PCM_STREAM_CAPTURE,0);
	if(ret < 0)
	{
		fprintf(stderr,"snd_pcm_open record faild\n");
		goto err;
	}

	ret = snd_pcm_open(&playback.handle,dev_name,SND_PCM_STREAM_PLAYBACK,0);
	if(ret < 0)
	{
		fprintf(stderr,"snd_pcm_open playbacke faild\n");
		goto err;
	}
	
	ret = SNDPCM_SetParams(&record);
	if(ret < 0)
	{
		fprintf(stderr,"SNDPCM_SetParams record faild\n");
		goto err;
	}	

	ret = SNDPCM_SetParams(&playback);
	if(ret < 0)
	{
		fprintf(stderr,"SNDPCM_SetParams playback faild\n");
		goto err;
	}


	ret = SNDPCM_Record_And_Play(&record,&playback);
	if(ret < 0)
	{
		fprintf(stderr,"SNDPCM_Record_And_Play faild\n");
		goto err;
	}

    free(record.data_buf);
	free(playback.data_buf);
    snd_pcm_close(record.handle);
	snd_pcm_close(playback.handle);
	return 0;

err:   
	if (record.data_buf) free(record.data_buf);
	if (playback.data_buf) free(playback.data_buf);
	if (record.handle) snd_pcm_close(record.handle);
	if (playback.handle) snd_pcm_close(playback.handle);
	return -1;

}

u_int32_t SND_WritePcm(SNDPCMContainer_t *playback,size_t wcount)
{
	ssize_t ret = 0;
	size_t  count = wcount;
	ssize_t result = 0;/*��Ϊ����ֵ*/
	char *temp_buf = playback->data_buf;

	if (wcount < playback->period_size) 
	{  
		/*�Ѳ�������������0���*/
//		fprintf(stderr, "wcount < playback->period_size\n");
        snd_pcm_format_set_silence(playback->format,temp_buf+ wcount * playback->bits_per_frame / 8,(playback->period_size-wcount)*playback->channels);  
        wcount = playback->period_size;  
    } 

	while(count > 0)
	{
		ret = snd_pcm_writei(playback->handle, temp_buf, count);
		if(ret == -EAGAIN || (ret > 0 && (size_t)ret < count))
		{
            snd_pcm_wait(playback->handle, 1000);  
        } 
		else if (ret == -EPIPE) 
		{  
            snd_pcm_prepare(playback->handle);  
            fprintf(stderr, "<<<<<<<<<<<<<<< Write Buffer Underrun >>>>>>>>>>>>>>>/n");  
        } 
		else if (ret == -ESTRPIPE) 
		{              
            fprintf(stderr, "<<<<<<<<<<<<<<< Write Need suspend >>>>>>>>>>>>>>>/n");          
        } 
		else if (ret < 0) 
		{  
            fprintf(stderr, "<<<<<<<<<<<<<<< write pcm faild >>>>>>>>>>>>>>>/n");  
            exit(-1);  
        }
		if(ret > 0)
		{
			result   += ret;
			count    -= ret;
			temp_buf += ret * playback->bits_per_frame / 8;
		}

	}

	return result;
}

u_int32_t SND_ReadPcm(SNDPCMContainer_t *record,size_t rcount)
{
	ssize_t ret = 0;
	size_t  count = rcount;
	ssize_t result = 0;/*��Ϊ����ֵ*/
	char *temp_buf = record->data_buf;
	
	if(count != record->period_size)/*��ֹ���δ���Ĭ��һ�ζ�ȡ�ĵ����ݴ�С����record->period_size*/
		count = record->period_size;
	
	while(count > 0)
	{
		ret = snd_pcm_readi(record->handle, temp_buf, count);
		if(ret == -EAGAIN || (ret > 0 && (size_t)ret < count))
		{
			snd_pcm_wait(record->handle, 1000);
		}
		else if(ret == -EPIPE)
		{
            snd_pcm_prepare(record->handle);  
            fprintf(stderr, "<<<<<<<<<<<<<<< Read Buffer Underrun >>>>>>>>>>>>>>>/n");			
		}
		else if(ret == -ESTRPIPE)
		{
			fprintf(stderr, "<<<<<<<<<<<<<<< Read suspend >>>>>>>>>>>>>>>/n");
		}
		else if(ret < 0)
		{
			fprintf(stderr, "<<<<<<<<<<<<<<< Read pcm faild >>>>>>>>>>>>>>>/n");
			exit(-1);
		}
		
		if(ret > 0)
		{
			result   += ret; /*��¼��ȡ��������*/	
			count    -= ret;/*ֱ����ȡ��count�����ݲ�����ѭ��*/
			temp_buf += ret * record->bits_per_frame / 8;/*ָ��յ��ֽڴ�*/
		}
	}

	return result;
}

/*���۴�PCM�ж�ȡ���ݻ���д�����ݣ���Щ���ݶ�����frameΪ��λ��*/
int SNDPCM_Record_And_Play(SNDPCMContainer_t *record,SNDPCMContainer_t *playback)
{
	u_int32_t ret = -1;
	size_t period_frames = 0;

	period_frames = record->period_byte * 8 / record->bits_per_frame;/*����һ����frameΪ��λ�����ݳ���*/
	
	while(1)
	{
		ret = SND_ReadPcm(record,period_frames);/*�ɼ�һ�����ڵ���������*/
		if(ret != period_frames)
		{
			fprintf(stderr,"SND_Read Pcm faild ret = %ld,period_frames = %d\n",ret,period_frames);
			return -1;
		}
		
		memcpy(playback->data_buf,record->data_buf,record->period_byte);/*����ȡ�������ݷŵ�palyback�Ļ�����ȥ*/
		
		ret = SND_WritePcm(playback,period_frames);/*���ɼ��������ݲ��ų���*/
		if(ret != period_frames)
		{
			fprintf(stderr,"SND_Write Pcm faild ret = %ld,period_frames = %d\n",ret,period_frames);
			return -1;
		}
		fprintf(stderr, ".");/*¼������*/
	}
	
	return 0;
}

int SNDPCM_SetParams(SNDPCMContainer_t *sndpcm)
{
	int ret = -1;
	u_int32_t exact_rate = 0;
	u_int32_t buffer_time = 0,period_time = 0;
	
	snd_pcm_hw_params_t *hwparams;/*����һ������ָ��*/
	

	snd_pcm_hw_params_alloca(&hwparams);/*Ϊhwparams�����ڴ棬����hwparamsָ����*/

	ret = snd_pcm_hw_params_any(sndpcm->handle, hwparams);/*ʹ��Ĭ�ϲ�����ʼ��hwparams*/
	if(ret < 0)
	{
		fprintf(stderr, "snd_pcm_hw_params_any faild\n");
		goto ERR_SET_PARAMS;
	}
	
	ret = snd_pcm_hw_params_set_access(sndpcm->handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);/*���÷���ģʽ��Ȩ��*/
	if(ret < 0)
	{
		fprintf(stderr, "snd_pcm_hw_params_set_access faild\n");
		goto ERR_SET_PARAMS;
	}

	ret = snd_pcm_hw_params_set_format(sndpcm->handle, hwparams, SND_PCM_FORMAT_S16_LE);/*���ò������ݸ�ʽ*/
	if(ret < 0)
	{
		fprintf(stderr, "snd_pcm_hw_params_set_format faild\n");
		goto ERR_SET_PARAMS;
	}	
	sndpcm->format = SND_PCM_FORMAT_S16_LE;/*�����ǵĽṹ���б���*/

	ret = snd_pcm_hw_params_set_channels(sndpcm->handle, hwparams, CHANNELS);/*����ͨ����Ϊ2 Stereo */
	if(ret < 0)
	{
		fprintf(stderr, "snd_pcm_hw_params_set_channels faild\n");
		goto ERR_SET_PARAMS;
	}
	sndpcm->channels = CHANNELS;

	exact_rate = SAMPLE_RATE;/*����������Ҫ�Ĳ������ʣ���Ӳ����֧����ѡ��һ����������������*/

	ret = snd_pcm_hw_params_set_rate_near(sndpcm->handle, hwparams, &exact_rate, 0);
	if(ret < 0)
	{
		fprintf(stderr, "snd_pcm_hw_params_set_rate_near faild\n");
		goto ERR_SET_PARAMS;
	}
	if(exact_rate != SAMPLE_RATE)
		fprintf(stderr, "The HardWare is not support our SAMPLE_RATE,exact_rate = %d\n",exact_rate);	

	ret = snd_pcm_hw_params_get_buffer_time_max(hwparams, &buffer_time, 0);/*��ȡĬ��buffer_time��ֵ*/
	if(ret < 0)
	{
		fprintf(stderr, "snd_pcm_hw_params_get_buffer_time_max faild\n");
		goto ERR_SET_PARAMS;		
	}
	buffer_time =  buffer_time > 500000 ? 500000 : buffer_time;
	period_time = buffer_time / 4;

	ret = snd_pcm_hw_params_set_buffer_time_near(sndpcm->handle, hwparams, &buffer_time, 0);/*����buffer_time��ֵ*/
	if(ret < 0)
	{
		fprintf(stderr, "snd_pcm_hw_params_set_buffer_time_near faild\n");
		goto ERR_SET_PARAMS;		
	}	

	ret = snd_pcm_hw_params_set_period_time_near(sndpcm->handle, hwparams, &period_time, 0);
	if(ret < 0)
	{
		fprintf(stderr, "snd_pcm_hw_params_set_period_time_near faild\n");
		goto ERR_SET_PARAMS;		
	}

	ret = snd_pcm_hw_params(sndpcm->handle, hwparams);/*���������õ�Ӳ������hwparams��Ч*/
	if(ret < 0)
	{
		fprintf(stderr, "snd_pcm_hw_params faild\n");
		goto ERR_SET_PARAMS;		
	}
	snd_pcm_hw_params_get_period_size(hwparams, &sndpcm->period_size,0);/*��ȡperiod_size��sndpcm->period_size*/
	snd_pcm_hw_params_get_buffer_size(hwparams, &sndpcm->buffer_size);/*��ȡbuffer_size��sndpcm->buffer_size*/

	if(sndpcm->period_size == sndpcm->buffer_size)
	{
		fprintf(stderr, "the sndpcm->period_size can't equal sndpcm->buffer_size\n");
		goto ERR_SET_PARAMS;		
	}
	
	sndpcm->bits_per_sample = snd_pcm_format_physical_width(sndpcm->format);/*����format�õ�һ�β���������λ��*/
	sndpcm->bits_per_frame   = sndpcm->bits_per_sample * sndpcm->channels;	/*��������λ����ͨ���������bit_per_frame*/
	sndpcm->period_byte     = sndpcm->period_size * sndpcm->bits_per_frame / 8;

	sndpcm->data_buf = (u_int8_t*)malloc(sndpcm->period_byte);/*����һ������������������С�պõ���һ�δ��������*/
	if(!sndpcm->data_buf)
	{
		fprintf(stderr, "malloc databuf memory faild\n");
		goto ERR_SET_PARAMS;
	}
	
	return 0;

ERR_SET_PARAMS:
	return -1;
	
}
















































