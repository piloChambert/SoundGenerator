#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

char *LoadWaveFile(const char *filename, int *channelCount, int *bps, size_t *size, int *freq) {
  FILE *fp;

  fp = fopen(filename,"r");
  if (fp) {
    //printf("Loading soundfile %s\n", file.c_str());
    char _id[5], *sound_buffer; //four bytes to hold 'RIFF'
    uint32_t _size; //32 bit value to hold file size
    uint16_t format_tag, channels, block_align, bits_per_sample; //our 16 values
    uint32_t format_length, sample_rate, avg_bytes_sec, data_size; //our 32 bit values
    
    fread(_id, sizeof(char), 4, fp); //read in first four bytes
    _id[4] = '\0';
    if (!strcmp(_id, "RIFF")) { //we had 'RIFF' let's continue
      fread(&_size, sizeof(uint32_t), 1, fp); //read in 32bit size value
      fread(_id, sizeof(char), 4, fp); //read in 4 byte string now
      
      if (!strcmp(_id,"WAVE")) { //this is probably a wave file since it contained "WAVE"
	fread(_id, sizeof(char), 4, fp); //read in 4 bytes "fmt ";
	fread(&format_length, sizeof(uint32_t),1,fp);
	fread(&format_tag, sizeof(uint16_t), 1, fp); //check mmreg.h (i think?) for other
	// possible format tags like ADPCM
	fread(&channels, sizeof(uint16_t),1,fp); //1 mono, 2 stereo
	fread(&sample_rate, sizeof(uint32_t), 1, fp); //like 44100, 22050, etc...
	fread(&avg_bytes_sec, sizeof(uint32_t), 1, fp); //probably won't need this
	fread(&block_align, sizeof(uint16_t), 1, fp); //probably won't need this
	fread(&bits_per_sample, sizeof(uint16_t), 1, fp); //8 bit or 16 bit file?
	fread(_id, sizeof(char), 4, fp); //read in 'data'
	fread(&data_size, sizeof(uint32_t), 1, fp); //how many bytes of sound data we have
	sound_buffer = new char[data_size]; //set aside sound buffer space
	fread(sound_buffer, sizeof(char), data_size, fp); //read in our whole sound data chunk

	if(channelCount != NULL)
	  *channelCount = channels;

	if(bps != NULL)
	  *bps = bits_per_sample;
	
	if(freq != NULL)
	  *freq = sample_rate;

	if(size != NULL)
	  *size = data_size;
	
	return sound_buffer;
      } else {
	printf("Error: RIFF file but not a wave file\n");
	return NULL;
      }
    } else {
      printf("Error: not a RIFF file\n");
      return NULL;
    }
  }
  printf("Couldn't load %s\n", filename);
  return NULL;
}

int main(int argc, char *argv[]) {
  
}
