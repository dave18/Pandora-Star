
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_thread.h>
//#include <SDL/SDL_mixer.h>
//#include <SDL_opengl.h>
#if defined (HAVE_GLES)
#include <GLES/gl.h>
#include "eglport.h"
#else
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glu.h>
#endif

#include <stdio.h>
#include <new>

/*#include <io.h>   // For access().
#include <sys/types.h>  // For stat().
#include <sys/stat.h>   // For stat().
#include <iostream>
*/
#include <stdlib.h>
//#include "bmfxml/bmfxml.h"
//#include "bmfont/render_bmfont.h"
#include <math.h>
#include <string.h>
#include <dirent.h>
#include "portaudio.h"
#include "FFTRealFixLen.h"
#include "bass.h"
#include "Singstar.h"
using namespace std;

const int REC_BUFFER=2048;

   FFTRealFixLen <12> fft_object; // 4096-point (2^12) FFT object constructed. 1024 buffer is 2^10 (2* 512)


#ifdef PLATFORM_GP2X
#include <unistd.h>
#endif // PLATFORM_GP2X

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

const int SCREEN_WIDTH  = 800;
const int SCREEN_HEIGHT = 480;

#if defined (HAVE_GLES)
#define GLdouble GLfloat
#define GL_CLAMP GL_CLAMP_TO_EDGE
#define glClearDepth glClearDepthf
#define glOrtho glOrthof
#define glColor4fv(a) glColor4f(a[0],a[1],a[2],a[3])
#define glColor3fv(a) glColor4f(a[0],a[1],a[2],1.0f)
#define glColor3f(a,b,c) glColor4f(a,b,c,1.0f)
#endif



#define PI 3.1415926536
#define MagCutoff 8.0
#define binsize 1.0/(REC_BUFFER*2.0/44100.0)

char LEFTDOWN,RIGHTDOWN,ADOWN,BDOWN,XDOWN,UPDOWN,DOWNDOWN,STARTDOWN,LSDOWN,RSDOWN,QDOWN=0;

int newdata;
char textbuffer[10];
int musicPlaying;
HRECORD micStream;
float * myData;
BASS_RECORDINFO recordinfo;
int recordchannels;

//**** Global Variable for Thread to Access
int quitSong;
int chosenSong;
int lineptr[2];
int fontPos[2];
float notespan;
float yp;
int lowPitch;
int highPitch;
int wordptr;
int relativeWordptr;
int noteIsActive;
inputStruct input[2];
int firstnote;
int lastnote;
int noteArea;
int pitchpos;
int vFontWidth;
float songpos;
int firstword;
float firstwordms;


// General purpose Ring-buffering routines
int SAMPLESIZE=1024;
#define BUFFSIZE 4096
#define NUM_BUFS 16

float hannWindow[REC_BUFFER*2];

sGlyph fontGlyph[2];
int numSongs;

vocalLines * vocals;
songDetails * songs;
player * playerDetails;

GLuint nocoverTexture;
GLuint borderTexture;
GLuint musiciconTexture;
GLuint videoiconTexture;
GLuint leftbarTexture;
GLuint midbarTexture;
GLuint rightbarTexture;
GLuint arrowsTexture;

//GlyphSet *pGlyphSet;

static void xtoa (
        unsigned long val,
        char *buf,
        unsigned radix,
        int is_neg
        )
{
        char *p;                /* pointer to traverse string */
        char *firstdig;         /* pointer to first digit */
        char temp;              /* temp char */
        unsigned digval;        /* value of digit */

        p = buf;

        if (is_neg) {
            /* negative, so output '-' and negate */
            *p++ = '-';
            val = (unsigned long)(-(long)val);
        }

        firstdig = p;           /* save pointer to first digit */

        do {
            digval = (unsigned) (val % radix);
            val /= radix;       /* get next digit */

            /* convert to ascii and store */
            if (digval > 9)
                *p++ = (char) (digval - 10 + 'a');  /* a letter */
            else
                *p++ = (char) (digval + '0');       /* a digit */
        } while (val > 0);
		*p=ENDCHAR;

        /* We now have the digit of the number in the buffer, but in reverse
           order.  Thus we reverse them now. */

        //*p-- = '\0';            /* terminate string; p points to last digit */
		*p--;

        do {
            temp = *p;
            *p = *firstdig;
            *firstdig = temp;   /* swap *p and *firstdig */
            --p;
            ++firstdig;         /* advance to next two digits */
        } while (firstdig < p); /* repeat until halfway */
}

/* Actual functions just call conversion helper with neg flag set correctly,
   and return pointer to buffer. */

char * nitoa (
        int val,
        char *buf,
        int radix
        )
{
        if (radix == 10 && val < 0)
            xtoa((unsigned long)val, buf, radix, 1);
        else
            xtoa((unsigned long)(unsigned int)val, buf, radix, 0);
        return buf;
}


void no_memory () {
  printf ("Failed to allocate memory!\n");
  exit (1);
}

int get_key()
{
    int keydata=0;
    SDL_Event event;
	while( SDL_PollEvent( &event ) )
		{

		//		if (event.type== SDL_JOYBUTTONDOWN)
			//	{														// GP2X buttons
				//}	//return event.jbutton.button;


		if (event.type==SDL_JOYBUTTONUP)
		{
			switch(event.jbutton.button)
			{
				case GP2X_BUTTON_UP:
							UPDOWN=0;
							break;
						case GP2X_BUTTON_LEFT:
							LEFTDOWN=0;
							break;
						case GP2X_BUTTON_RIGHT:
							RIGHTDOWN=0;
							break;
                        case GP2X_BUTTON_DOWN:
							DOWNDOWN=0;
							break;
                        case GP2X_BUTTON_A:
							ADOWN=0;
							break;
                        case GP2X_BUTTON_B:
							BDOWN=0;
							break;
                        case GP2X_BUTTON_X:
							XDOWN=0;
							break;
                        case GP2X_BUTTON_R:
							RSDOWN=0;
							break;
                        case GP2X_BUTTON_L:
							LSDOWN=0;
							break;
                        case GP2X_BUTTON_START:
                            STARTDOWN=0;
							break;
						default:
							break;
                }
        }

        if (event.type== SDL_JOYBUTTONDOWN)
        {																	// GP2X buttons

            switch( event.jbutton.button )
            {
						case GP2X_BUTTON_UP:
							UPDOWN=1;
							break;
						case GP2X_BUTTON_LEFT:
							LEFTDOWN=1;
							break;
						case GP2X_BUTTON_RIGHT:
							RIGHTDOWN=1;
							break;
                        case GP2X_BUTTON_DOWN:
							DOWNDOWN=1;
							break;
                        case GP2X_BUTTON_A:
							ADOWN=1;
							break;
                        case GP2X_BUTTON_B:
							BDOWN=1;
							break;
                        case GP2X_BUTTON_X:
							XDOWN=1;
							break;
                        case GP2X_BUTTON_R:
							RSDOWN=1;
							break;
                        case GP2X_BUTTON_L:
							LSDOWN=1;
							break;
                        case GP2X_BUTTON_START:
                            STARTDOWN=1;
							break;
						default:
							break;
            }
        }




                if (event.type== SDL_KEYUP)
                {																	// PC buttons

					switch( event.key.keysym.sym )
					{
						case SDLK_UP:
							UPDOWN=0;
							break;
						case SDLK_LEFT:
							LEFTDOWN=0;
							break;
						case SDLK_RIGHT:
							RIGHTDOWN=0;
							break;
                        case SDLK_DOWN:
							DOWNDOWN=0;
							break;
                        case SDLK_HOME:
							ADOWN=0;
							break;
                        case SDLK_END:
							BDOWN=0;
							break;
                        case SDLK_LALT:
                            STARTDOWN=0;
							break;
                        case SDLK_RSHIFT:
                            LSDOWN=0;
							break;
                        case SDLK_q:
                            QDOWN=0;
							break;
						default:
							break;
					}
                }

				if (event.type== SDL_KEYDOWN)
				{																	// PC buttons

					switch( event.key.keysym.sym )
					{
						case SDLK_UP:
							UPDOWN=1;
							break;
						case SDLK_LEFT:
							LEFTDOWN=1;
							break;
						case SDLK_RIGHT:
							RIGHTDOWN=1;
							break;
                        case SDLK_DOWN:
							DOWNDOWN=1;
							break;
                        case SDLK_HOME:
							ADOWN=1;
							break;
                        case SDLK_END:
							BDOWN=1;
							break;
                        case SDLK_LALT:
                            STARTDOWN=1;
							break;
                        case SDLK_RSHIFT:
                            LSDOWN=1;
							break;
                        case SDLK_q:
                            QDOWN=1;
							break;
						default:
							break;
					}
				}

			}

		if (UPDOWN) keydata|=MY_UP;
		if (LEFTDOWN) keydata|=MY_LEFT;
		if (RIGHTDOWN) keydata|=MY_RIGHT;
		if (DOWNDOWN) keydata|=MY_DOWN;
		if (ADOWN) keydata|=MY_BUTT_A;
		if (BDOWN) keydata|=MY_BUTT_B;
		if (XDOWN) keydata|=MY_BUTT_X;
		if (LSDOWN) keydata|=MY_BUTT_SL;
		if (RSDOWN) keydata|=MY_BUTT_SR;
		if (STARTDOWN) keydata|=MY_START;
		if (QDOWN) keydata|=MY_QT;
        return keydata;
}



void SetOpenGLAtributes(void)
{
    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE,8 );
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32 );
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);

}

BOOL CALLBACK micCallback(HRECORD handle, const void *inputBuffer,
                           DWORD framesPerBuffer,void *userData )
                           {
                                //memcpy((char*)userData,(char*)inputBuffer,4096<<2);
                                float *indata = (float *)userData;
                                float * buffdata = (float*) inputBuffer;
                                unsigned int i;
                                for (i=0;i<framesPerBuffer<<1;i++)
                                {
                                    *indata++=*buffdata++;

                                  //  *indata=*buffdata;
                                  //  *(indata+framesPerBuffer)=*(buffdata+1);
                                  //  indata+=2;
                                   // buffdata+=2;

                                    //*indata=*buffdata;
                                    //*(indata+(framesPerBuffer<<1))=*(buffdata+2);
                                    //indata++;
                                    //buffdata+=3;
                                }


                                newdata=1;
                                //printf("Ticks: %d\n",SDL_GetTicks());
                               return 0;

                           }



void createHannWindow(void)
{
    int z;
    for (z=0;z<REC_BUFFER*2;z++)
    {
        //hannWindow[z]=0.5*(1-cos((2.0*PI*(float)z)/4095));
        hannWindow[z]=0.54 - 0.46 *cos((2.0*PI*(float)z)/4095);
    }
}

void musicFinished()
{
	musicPlaying = 0;
}


int singSong(void * unused)
{
  //  FILE * testleftoutput;
 //   FILE * testrightoutput;

    int z,zz;

   // int relativeWordptr;
    float xp;
    //float pitches[2] [500];


    float pitchUpdate;
    float pitchctr;
    int wordcount;

  //  int modeArray[88];


    newdata=0;


    lineptr[0]=0;
    wordptr=0;

    //Get first line of song
    fontPos[0]=0;
    fontPos[1]=0;
    lowPitch=200;
    highPitch=0;
    z=lineptr[0];
    firstnote=vocals[z].beat;
    lastnote=vocals[z].beat+vocals[z].length;
    pitchpos=0;
    pitchctr=0;
    relativeWordptr=0;

    wordcount=0;

    while (vocals[z].type>0)
    {
        fontPos[0]+=vFontWidth * strlen(vocals[z].lyrics);
        if (vocals[z].pitch<lowPitch) lowPitch=vocals[z].pitch;
        if (vocals[z].pitch>highPitch) highPitch=vocals[z].pitch;
        lastnote=vocals[z].beat+vocals[z].length;
        z++;
        wordcount++;
    }
    fontPos[0]=400-(fontPos[0]/2);
    z++;
    lineptr[1]=z;
    while (vocals[z].type>0)
    {
        fontPos[1]+=vFontWidth * strlen(vocals[z].lyrics);
        z++;
    }
    fontPos[1]=400-(fontPos[1]/2);
    notespan=(float)noteArea/(float)(lastnote-firstnote);
    yp=200+(float)(highPitch-lowPitch)/2.0*16.0;
    firstwordms=vocals[wordptr].ms;
    firstword=0;
    noteIsActive=0;

//    memset(playerDetails[0].modeArray,0,88);
//    memset(playerDetails[1].modeArray,0,88);

    playerDetails[0].playerResults=new results[wordcount];
    playerDetails[1].playerResults=new results[wordcount];
    for (z=0;z<wordcount;z++)
    {
        playerDetails[0].playerResults[z].recordedPitches =new int[vocals[wordptr+z].scoringSegments];
        playerDetails[1].playerResults[z].recordedPitches =new int[vocals[wordptr+z].scoringSegments];
  //      printf("Segments catered for word %d: %d\n",z+1,vocals[wordptr+z].scoringSegments);
    }



    //pitchUpdate=(songs[chosenSong].mspb/4.0)/notespan;


    if(BASS_ChannelPlay(music, true) == false)
    {
        printf("Unable to play music file: %d\n", BASS_ErrorGetCode());
        BASS_RecordFree();
        return 1;
    }

     while ((quitSong==0)  && (musicPlaying))
    {

        if (newdata)
        {
            for (z=0;z<REC_BUFFER*2;z++)
            {
                input[0].x[z]=*(myData+(z<<1))*hannWindow[z];
                input[1].x[z]=*(myData+(z<<1)+1)*hannWindow[z];
            }

            fft_object.do_fft (input[0].f,input[0].x);
            fft_object.do_fft (input[1].f,input[1].x);

            input[0].highserial=0;
            input[1].highserial=0;
            input[0].highmag=0;
            input[1].highmag=0;
            input[0].mag[0]=input[0].f[0];
            input[1].mag[0]=input[1].f[0];
            for (z=1;z<REC_BUFFER;z++)
            {
                input[0].mag[z]=sqrt((input[0].f[z]*input[0].f[z])+(input[0].f[z+REC_BUFFER]*input[0].f[z+REC_BUFFER]));
                input[1].mag[z]=sqrt((input[1].f[z]*input[1].f[z])+(input[1].f[z+REC_BUFFER]*input[1].f[z+REC_BUFFER]));
                if (input[0].mag[z]>input[0].highmag)
                {
                    input[0].highmag=input[0].mag[z];
                    input[0].highserial=z;
                }
                if (input[1].mag[z]>input[1].highmag)
                {
                    input[1].highmag=input[1].mag[z];
                    input[1].highserial=z;
                }
            }

            input[0].freq=(float)input[0].highserial*binsize;
            input[1].freq=(float)input[1].highserial*binsize;

            if (input[0].highserial>0) input[0].intserial=(float)input[0].highserial + (input[0].mag[input[0].highserial+1] - input[0].mag[input[0].highserial-1])/(2*(2*input[0].mag[input[0].highserial] - input[0].mag[input[0].highserial-1] - input[0].mag[input[0].highserial+1])); else input[0].intserial=(float)input[0].highserial;
            input[0].intfreq=input[0].intserial*binsize;


            if (input[1].highserial>0) input[1].intserial=(float)input[1].highserial + (input[1].mag[input[1].highserial+1] - input[1].mag[input[1].highserial-1])/(2*(2*input[1].mag[input[1].highserial] - input[1].mag[input[1].highserial-1] - input[1].mag[input[1].highserial+1])); else input[1].intserial=(float)input[1].highserial;
            input[1].intfreq=input[1].intserial*binsize;

            //printf ("Pitch[0]: %f      Pitch[1]: %f \n",input[0].intfreq,input[1].intfreq);

            if (input[0].highmag>MagCutoff) input[0].reducedfreq=input[0].intfreq; else input[0].reducedfreq=0;
            //printf ("Pitch: %f \n",input[0].reducedfreq);

            if (input[1].highmag>MagCutoff) input[1].reducedfreq=input[1].intfreq; else input[1].reducedfreq=0;

            if (input[0].reducedfreq==0) input[0].pitch=0; else input[0].pitch=(int)round(12*log2(input[0].intfreq/base_a4)+57.0);
            if (input[1].reducedfreq==0) input[1].pitch=0; else input[1].pitch=(int)round(12*log2(input[1].intfreq/base_a4)+57.0);

            printf ("Pitch[0]: %d      Pitch[1]: %d \n",input[playerDetails[0].inputChannel].pitch,input[playerDetails[1].inputChannel].pitch);

            //***If we are recording sung pitch as 20 sec granularity there is no time to build up an array of pitches so may as well just use current pitch
            //if ((input[playerDetails[0].inputChannel].pitch>=0) && (input[playerDetails[0].inputChannel].pitch<88)) playerDetails[0].modeArray[input[playerDetails[0].inputChannel].pitch]++; //increase modeArray
            //if ((input[playerDetails[1].inputChannel].pitch>=0) && (input[playerDetails[1].inputChannel].pitch<88)) playerDetails[1].modeArray[input[playerDetails[1].inputChannel].pitch]++;
            if (input[playerDetails[0].inputChannel].pitch>0)
            {
                playerDetails[0].currentPitch=input[playerDetails[0].inputChannel].pitch;
                while (playerDetails[0].currentPitch<vocals[wordptr].pitch-6.0) playerDetails[0].currentPitch+=12;
                while (playerDetails[0].currentPitch>vocals[wordptr].pitch+6.0) playerDetails[0].currentPitch-=12;
            }
            else playerDetails[0].currentPitch=0;

            if (input[playerDetails[1].inputChannel].pitch>0)
            {
                playerDetails[1].currentPitch=input[playerDetails[1].inputChannel].pitch;
                while (playerDetails[1].currentPitch<vocals[wordptr].pitch-6.0) playerDetails[1].currentPitch+=12;
                while (playerDetails[1].currentPitch>vocals[wordptr].pitch+6.0) playerDetails[1].currentPitch-=12;
            }
            else playerDetails[1].currentPitch=0;


             newdata=0;
        }




        QWORD cpos=BASS_ChannelGetPosition(music,BASS_POS_BYTE);
        songpos=BASS_ChannelBytes2Seconds(music,cpos)*1000;

        if (noteIsActive)
        {
            if (songpos>pitchctr+scoreGranularity)
            {

                pitchctr=songpos;
              /*  //Get mode average pitch;
                int p1p,p1h,p2p,p2h;
                p1p=0;p1h=0;p2p=0;p2h=0;
                for (z=0;z<88;z++)
                {
                    if (playerDetails[0].modeArray[z]>p1h) {p1h=playerDetails[0].modeArray[z];p1p=z;}
                    if (playerDetails[1].modeArray[z]>p2h) {p2h=playerDetails[1].modeArray[z];p2p=z;}
                }
                //Adjust pitch to correct octave
                while (p1p<vocals[wordptr].pitch-6.0) p1p+=12;
                while (p1p>vocals[wordptr].pitch+6.0) p1p-=12;
                while (p2p<vocals[wordptr].pitch-6.0) p2p+=12;
                while (p2p>vocals[wordptr].pitch+6.0) p2p-=12;

                playerDetails[0].playerResults[relativeWordptr].recordedPitches[pitchpos]=p1p;
                playerDetails[1].playerResults[relativeWordptr].recordedPitches[pitchpos]=p2p;*/

                playerDetails[0].playerResults[relativeWordptr].recordedPitches[pitchpos]=playerDetails[0].currentPitch;
                playerDetails[1].playerResults[relativeWordptr].recordedPitches[pitchpos]=playerDetails[1].currentPitch;

                //playerDetails[1].playerResults[relativeWordptr].recordedPitches[pitchpos]=vocals[wordptr].pitch; //temp code to force correct pitch for testing

           //     if (p1p==vocals[wordptr].pitch) playerDetails[0].score+=songs[chosenSong].scorePerSegment;
         //       if (p2p==vocals[wordptr].pitch) playerDetails[1].score+=songs[chosenSong].scorePerSegment;
                printf("Rel:%d scoring seg:%d pitchpos:%d correct pitch:%d recorded pitch:%d\n",relativeWordptr,vocals[wordptr].scoringSegments,pitchpos,vocals[wordptr].pitch,playerDetails[0].currentPitch);
                if (pitchpos<vocals[wordptr].scoringSegments-1) pitchpos++;
            }
        }

       // if (songpos>vocals[wordptr].ms+vocals[wordptr].length*songs[chosenSong].mspb+(float)songs[chosenSong].gap) noteIsActive=0;


        if  (songpos>=vocals[wordptr].ms+songpos>=vocals[wordptr].lengthms+(float)songs[chosenSong].gap) noteIsActive=0;

        if  (songpos>=vocals[wordptr+1].ms+(float)songs[chosenSong].gap)
        {
            wordptr++;
            if (firstword) relativeWordptr++;
            noteIsActive=1;
            if (vocals[wordptr].type==0)
            {
            //    printf("Wordcount for deletion: %d\n",wordcount);
                for (z=0;z<wordcount;z++)
                {
                    delete[] playerDetails[0].playerResults[z].recordedPitches;
                    delete[] playerDetails[1].playerResults[z].recordedPitches;
                }
                delete[] playerDetails[0].playerResults;
                delete[] playerDetails[1].playerResults;
                lineptr[0]=wordptr+1;

                //Get next line of song
                fontPos[0]=0;
                fontPos[1]=0;
                lowPitch=200;
                highPitch=0;
                z=lineptr[0];
                firstnote=vocals[z].beat;
                lastnote=vocals[z].beat+vocals[z].length;
                wordcount=0;
                while (vocals[z].type>0)
                {
                    fontPos[0]+=vFontWidth * strlen(vocals[z].lyrics);
                    if (vocals[z].pitch<lowPitch) lowPitch=vocals[z].pitch;
                    if (vocals[z].pitch>highPitch) highPitch=vocals[z].pitch;
                    lastnote=vocals[z].beat+vocals[z].length;
                    z++;
                    wordcount++;
                }
                fontPos[0]=400-(fontPos[0]/2);
                z++;
                lineptr[1]=z;
                while (vocals[z].type>0)
                {
                    fontPos[1]+=vFontWidth * strlen(vocals[z].lyrics);
                    z++;
                }
                fontPos[1]=400-(fontPos[1]/2);
                notespan=(float)noteArea/(float)(lastnote-firstnote);
                yp=200+(float)(highPitch-lowPitch)/2.0*16.0;
                firstwordms=vocals[wordptr+1].ms;
                firstword=0;
                noteIsActive=0;

//                memset(playerDetails[0].modeArray,0,88);
//                memset(playerDetails[1].modeArray,0,88);

                printf("Wordcount for creation: %d\n",wordcount);

                playerDetails[0].playerResults=new results[wordcount];
                playerDetails[1].playerResults=new results[wordcount];
                for (z=0;z<wordcount;z++)
                {
                    playerDetails[0].playerResults[z].recordedPitches =new int[vocals[wordptr+z+1].scoringSegments];
                    playerDetails[1].playerResults[z].recordedPitches =new int[vocals[wordptr+z+1].scoringSegments];
                  //  printf("Segments catered for word %d: %d\n",z+1,vocals[wordptr+z+1].scoringSegments);
                }

             //   //pitchUpdate=(songs[chosenSong].mspb/4.0)/notespan;
             //   pitchUpdate=(songs[chosenSong].mspb/4.0)/notespan;
             //   printf("mspb: %f firstnote: %d lastnote: %d notespan: %f pitchupdate: %f\n",songs[chosenSong].mspb,firstnote,lastnote,notespan,pitchUpdate);
            }
            else
            {

                pitchpos=0;
                pitchctr=songpos;
/*                for (z=0;z<vocals[wordptr].scoringSegments;z++)
                {
                    playerDetails[0].playerResults[relativeWordptr].recordedPitches[z]=0;
                    playerDetails[1].playerResults[relativeWordptr].recordedPitches[z]=0;
                }
*/
            }
        }

        if  ((songpos>=firstwordms+(float)songs[chosenSong].gap) && (firstword==0))
        {
            firstword=1;
            pitchctr=songpos;
            relativeWordptr=0;
            noteIsActive=1;
        }




       // SDL_Delay(0);



    }

    for (z=0;z<wordcount;z++)
    {
        delete[] playerDetails[0].playerResults[z].recordedPitches;
        delete[] playerDetails[1].playerResults[z].recordedPitches;
    }
    delete[] playerDetails[0].playerResults;
    delete[] playerDetails[1].playerResults;

    quitSong=2;
    return 0;
}

int call_song(void)
{

    SDL_Rect pitchBar;

    pitchBar.x=400;
    pitchBar.w=64;
    pitchBar.h=16;

    int z,zz,err,keydata;
    float xp;



    GLuint bartexture=SurfacetoTexture((char*)"bar.bmp");

    musicPlaying=0;

    BASS_ChannelPlay(micStream,true);
    //printf(  "BASS Mic channel play message: %s\n", err  );
    //if (err!=0) return 1;

    //   Mix_HookMusicFinished(musicFinished);
    musicPlaying=1;

    long ticksatstart;

    vFontWidth=16;
    noteArea=720; //Screen width that note bars can occupy

    SDL_Thread *thread;

    thread = SDL_CreateThread(singSong, NULL);
    if ( thread == NULL ) {
        printf("Unable to create thread: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Delay(200);



    while ((quitSong<2) && (musicPlaying))
    {

        ticksatstart=SDL_GetTicks();
        glClearColor(1.0, 1.0, 1.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


        // Display lyrics and Pitch Bars
        z=lineptr[0];
        xp=(float)fontPos[0];
        int count=0;
        while (vocals[z].type>0)
        {
            float segspan=((float)vocals[z].length*notespan)/(float)vocals[z].scoringSegments;
            putTexture(leftbarTexture,40+(vocals[z].beat-firstnote)*notespan,yp-((vocals[z].pitch-lowPitch)*16),segspan,16.0,1,1,1);
            if (vocals[z].scoringSegments>2)
            {
                float r,g,b;
                for (zz=1;zz<vocals[z].scoringSegments;zz++)
                {
                    r=1;g=1;b=1;
                    putTexture(midbarTexture,(40+(vocals[z].beat-firstnote)*notespan)+((float)zz*segspan),yp-((vocals[z].pitch-lowPitch)*16),segspan,16.0,r,g,b);
                    if ((z<=wordptr) && (firstword))
                    {
                        if ((z<wordptr) || (zz<pitchpos)) //only analyse recorded segments for previous words or previous segments of current word
                        {
                            //printf("z:%d  count:%d  zz:%d  songpitch:%d   vocalpitch:%d  pitchpos:%d   %s\n",z,count,zz,vocals[z].pitch,playerDetails[0].playerResults[count].recordedPitches[zz],pitchpos,vocals[z].lyrics);
                            r=0; //show player one pitch
                            g=0;
                            putTexture(midbarTexture,(40+(vocals[z].beat-firstnote)*notespan)+((float)zz*segspan),yp-((playerDetails[0].playerResults[count].recordedPitches[zz]-lowPitch)*16),segspan,16.0,r,g,b);
                            b=0; //show player one pitch
                            g=0;
                            r=1;
                            putTexture(midbarTexture,(40+(vocals[z].beat-firstnote)*notespan)+((float)zz*segspan),yp-((playerDetails[1].playerResults[count].recordedPitches[zz]-lowPitch)*16),segspan,16.0,r,g,b);
                            //if (playerDetails[0].playerResults[count].recordedPitches[zz]==vocals[z].pitch) {r=0;g=0;}
                            //if (playerDetails[1].playerResults[count].recordedPitches[zz]==vocals[z].pitch) {b=0;g=0;}
                            //if ((r==0) && (b==0) && (g==0)) {r=1;b=1;g=0;}
                        }
                    }
                    //putTexture(midbarTexture,(40+(vocals[z].beat-firstnote)*notespan)+((float)zz*segspan),yp-((vocals[z].pitch-lowPitch)*16),segspan,16.0,r,g,b);
                }
            }
            if (vocals[z].scoringSegments>1)
            {
                putTexture(rightbarTexture,(40+(vocals[z].beat-firstnote)*notespan)+((float)vocals[z].scoringSegments*segspan),yp-((vocals[z].pitch-lowPitch)*16),segspan,16.0,1,1,1);
            }
            else
            {
                putTexture(rightbarTexture,40+(vocals[z].beat-firstnote)*notespan+8,yp-((vocals[z].pitch-lowPitch)*16),segspan,16.0,1,1,1);
            }

            if ((z==wordptr)  && (noteIsActive))
            {

                if (input[0].pitch>0)
                {
                    input[0].adjpitch=input[0].pitch; //Easy mode?? always adjusts to the nearest note
                    while (input[0].adjpitch<vocals[z].pitch-6.0) input[0].adjpitch+=12;
                    while (input[0].adjpitch>vocals[z].pitch+6.0) input[0].adjpitch-=12;
                    putTexture(bartexture,56+(vocals[z].beat-firstnote)*notespan,yp-((input[0].adjpitch-lowPitch)*16),vocals[z].length*notespan-32,16.0,0,0,1);
                } else input[0].adjpitch=0;
                //adjpitches[0] [pitchpos]=input[0].adjpitch;

                if (input[1].pitch>0)
                {
                    input[1].adjpitch=input[1].pitch;
                    while (input[1].adjpitch<vocals[z].pitch-6.0) input[1].adjpitch+=12;
                    while (input[1].adjpitch>vocals[z].pitch+6.0) input[1].adjpitch-=12;
                    putTexture(bartexture,56+(vocals[z].beat-firstnote)*notespan,yp-((input[1].adjpitch-lowPitch)*16),vocals[z].length*notespan-32,16.0,1,0,0);
                } else input[1].adjpitch=0;
                //adjpitches[1] [pitchpos]=input[1].adjpitch;

               xp=xp+putFont(&fontGlyph[1],xp,420,vFontWidth*1.2,20*1.2,vocals[z].lyrics,1,1,1);


            }
            else xp=xp+putFont(&fontGlyph[1],xp,420,vFontWidth,20,vocals[z].lyrics,1,1,1);
           z++;
           count++;
        }
        z=lineptr[1];
        xp=(float)fontPos[1];
        while (vocals[z].type>0)
        {
            xp=xp+putFont(&fontGlyph[1],xp,450,vFontWidth,20,vocals[z].lyrics,1,1,1);

            z++;
        }

     /*   if (firstword==0) //We need to show new line marker
        {
            xp=(float)fontPos[0]/2000.0*(firstwordms+(float)songs[chosenSong].gap-songpos);
            if (songpos>firstwordms+(float)songs[chosenSong].gap-2000.0) putTexture(arrowsTexture,(float)fontPos[0]-xp,420,16,16.0,1,1,1);
            if (songpos<=firstwordms+(float)songs[chosenSong].gap-2000.0) putTexture(arrowsTexture,0,420,16,16.0,1,1,1);
        }
*/
        /*xp=notespan/2;
        for (z=0;z<=pitchpos;z++)
        {
            if (adjpitches[0] [z]>0) putTexture(bartexture,40+z,yp-((adjpitches[0] [z]-lowPitch)*16),1,16.0,0,0,1);
            if (adjpitches[1] [z]>0) putTexture(bartexture,40+z,yp-((adjpitches[1] [z]-lowPitch)*16),1,16.0,1,0,0);
        }*/

        nitoa((int)(songpos/1000.0),textbuffer,10);
        putFont(&fontGlyph[1],0.0,190.0,12.0,16.0,(char*)"Time Elapsed:",1,1,1);
        putFont(&fontGlyph[1],300.0,190.0,12.0,16.0,textbuffer,1,1,1);


        nitoa((int)playerDetails[0].currentPitch,textbuffer,10);
       // if (pitchpos>0) nitoa((int)playerDetails[0].playerResults[relativeWordptr].recordedPitches[pitchpos-1],textbuffer,10);
        putFont(&fontGlyph[1],0.0,270.0,12.0,16.0,(char*)"Adj Pitch:",1,1,1);
        putFont(&fontGlyph[1],300.0,270.0,12.0,16.0,textbuffer,1,1,1);


       // putFont(&fontGlyph[1],0.0,270.0,12.0,16.0,"Adj Pitch:",1,1,1);
      /*  if (pitchpos>0)
        {
        if  ((playerDetails[0].playerResults[relativeWordptr].recordedPitches[pitchpos-1]>0) && (playerDetails[0].playerResults[relativeWordptr].recordedPitches[pitchpos-1]<100))
        {
            nitoa((int)playerDetails[0].playerResults[relativeWordptr].recordedPitches[pitchpos-1],textbuffer,10);
     //   nitoa((int)input[0].adjpitch,textbuffer,10);
            putFont(&fontGlyph[1],300.0,270.0,12.0,16.0,textbuffer,1,1,1);
        }
        }
*/
        nitoa(vocals[wordptr].pitch,textbuffer,10);
        putFont(&fontGlyph[1],0.0,290.0,12.0,16.0,(char*)"Actual Pitch:",1,1,1);
        putFont(&fontGlyph[1],300.0,290.0,12.0,16.0,textbuffer,1,1,1);

        nitoa(highPitch,textbuffer,10);
        putFont(&fontGlyph[1],0.0,310.0,12.0,16.0,(char*)"High Pitch:",1,1,1);
        putFont(&fontGlyph[1],300.0,310.0,12.0,16.0,textbuffer,1,1,1);

        nitoa(lowPitch,textbuffer,10);
        putFont(&fontGlyph[1],0.0,330.0,12.0,16.0,(char*)"Low Pitch:",1,1,1);
        putFont(&fontGlyph[1],300.0,330.0,12.0,16.0,textbuffer,1,1,1);

        long timetaken=SDL_GetTicks()-ticksatstart;
        nitoa((int)timetaken,textbuffer,10);
        putFont(&fontGlyph[1],0.0,210.0,12.0,16.0,(char*)"ticks:",1,1,1);
        putFont(&fontGlyph[1],100.0,210.0,12.0,16.0,textbuffer,1,1,1);

        printf("Milisec taken: %d\n",(int)(SDL_GetTicks()-ticksatstart));
/*
        pitchBar.y=400-((input[0].pitch-50)*16);;
//        SDL_FillRect(screen, &pitchBar, SDL_MapRGB(screen->format, 64,64,196));
        putTexture(bartexture,400.0,pitchBar.y,128.0,16.0,1,0,0);

        pitchBar.y=400-((input[1].pitch-50)*16);
//        SDL_FillRect(screen, &pitchBar, SDL_MapRGB(screen->format, 196,64,64));
        putTexture(bartexture,400.0,pitchBar.y,128.0,16.0,0,0,1);
/*/
        #if defined (HAVE_GLES)
        EGL_SwapBuffers();
        #else
        SDL_GL_SwapBuffers(); //HW DB waits for vertical blank.
        #endif

        keydata=get_key();
        if ((keydata==MY_QT) && (quitSong==0)) quitSong=1;

        if (BASS_ChannelIsActive(music)==BASS_ACTIVE_STOPPED) musicPlaying=0;

        if (quitSong==1) SDL_WaitThread(thread, NULL);

    }



    if (musicPlaying) BASS_ChannelStop(music);

    BASS_ChannelStop(micStream);
    BASS_RecordFree();
    //err=Pa_StopStream(micStream);
    //printf(  "PortAudio StopStream message: %s\n", Pa_GetErrorText( err ) );
    //err=Pa_CloseStream(micStream);
    //printf(  "PortAudio CloseStream message: %s\n", Pa_GetErrorText( err ) );

    return 0;
}

int file_length(char * fl)
{
int pos;
int end;
FILE * f;

f=fopen (fl,"r+b");
if (f==NULL) return -2;
rewind(f);
pos = ftell (f);
if (pos==-1) return -1;
fseek (f, 0, SEEK_END);
end = ftell (f);
fseek (f, pos, SEEK_SET);
fclose (f);

return end;
}

void parseUltrastar(char * filename)
{
    FILE * songfile;
    char temp[128];
    int lns=0;
    char c,c2;
    char leadspace;
    char trailspace;
    char tempword[50];
    int spaces;

    songs[chosenSong].numVocals=1;





    songfile=fopen (filename,"r");
    if (songfile!=NULL)
    {
        // First we need to count entries to size array
        while (!(feof(songfile)))
        {
            fgets (temp , 128 , songfile);
            if ((temp[0]==':') || (temp[0]=='-') || (temp[0]=='*') || (temp[0]=='F')) lns++;
        }

        rewind(songfile);

        printf("Lines found: %d\n",lns);
        vocals=new vocalLines[lns];

        //Now to parse the file
        lns=0;
        while (!(feof(songfile)))
        {
            fgets (temp , 128 , songfile);
            if (temp[0]==':')
            {
                memset(tempword,0,50);
                sscanf(temp,"%c %d %d %d%c%c%s%c",&c,&vocals[lns].beat,&vocals[lns].length,&vocals[lns].pitch,&c2,&leadspace,tempword,&trailspace);
                spaces=0;
                if (leadspace==' ') spaces++;
                if (trailspace==' ') spaces++;
                vocals[lns].lyrics=(char*)malloc(strlen(tempword)+1+spaces);
                if (leadspace==' ')
                {
                    strcpy(vocals[lns].lyrics," ");
                }
                else
                {
                    vocals[lns].lyrics[0]=leadspace;
                    vocals[lns].lyrics[1]=0;
                }
                strcat(vocals[lns].lyrics,tempword);

                if (trailspace==' ') strcat(vocals[lns].lyrics," ");
           //     printf("%s\n",vocals[lns].lyrics);

                vocals[lns].type=1;
                vocals[lns].ms=1000.0/(songs[chosenSong].bpm/15.0)*(float)vocals[lns].beat;
                vocals[lns].lengthms=1000.0/(songs[chosenSong].bpm/15.0)*(float)vocals[lns].length;
                //vocals[lns].scoringSegments=(int)(((float)vocals[lns].length*songs[chosenSong].mspb)/scoreGranularity)+0.5;
                vocals[lns].scoringSegments=(int)ceil(vocals[lns].lengthms/scoreGranularity);

                lns++;
            }
            if (temp[0]=='F')
            {
                memset(tempword,0,50);
                sscanf(temp,"%c %d %d %d%c%c%s%c",&c,&vocals[lns].beat,&vocals[lns].length,&vocals[lns].pitch,&c2,&leadspace,tempword,&trailspace);
                spaces=0;
                if (leadspace==' ') spaces++;
                if (trailspace==' ') spaces++;
                vocals[lns].lyrics=(char*)malloc(strlen(tempword)+1+spaces);
                if (leadspace==' ')
                {
                    strcpy(vocals[lns].lyrics," ");
                }
                else
                {
                    vocals[lns].lyrics[0]=leadspace;
                    vocals[lns].lyrics[1]=0;
                }
                strcat(vocals[lns].lyrics,tempword);

                if (trailspace==' ') strcat(vocals[lns].lyrics," ");
          //      printf("%s\n",vocals[lns].lyrics);

                vocals[lns].type=2;
                vocals[lns].ms=1000.0/(songs[chosenSong].bpm/15.0)*(float)vocals[lns].beat;
                vocals[lns].lengthms=1000.0/(songs[chosenSong].bpm/15.0)*(float)vocals[lns].length;
                //vocals[lns].scoringSegments=(int)(((float)vocals[lns].length*songs[chosenSong].mspb)/scoreGranularity)+0.5;
                vocals[lns].scoringSegments=(int)ceil(vocals[lns].lengthms/scoreGranularity);
                lns++;
            }
            if (temp[0]=='*')
            {
                 memset(tempword,0,50);
                sscanf(temp,"%c %d %d %d%c%c%s%c",&c,&vocals[lns].beat,&vocals[lns].length,&vocals[lns].pitch,&c2,&leadspace,tempword,&trailspace);
                spaces=0;
                if (leadspace==' ') spaces++;
                if (trailspace==' ') spaces++;
                vocals[lns].lyrics=(char*)malloc(strlen(tempword)+1+spaces);
                if (leadspace==' ')
                {
                    strcpy(vocals[lns].lyrics," ");
                }
                else
                {
                    vocals[lns].lyrics[0]=leadspace;
                    vocals[lns].lyrics[1]=0;
                }
                strcat(vocals[lns].lyrics,tempword);

                if (trailspace==' ') strcat(vocals[lns].lyrics," ");
        //        printf("%s\n",vocals[lns].lyrics);

                vocals[lns].type=3;
                vocals[lns].ms=1000.0/(songs[chosenSong].bpm/15.0)*(float)vocals[lns].beat;
                vocals[lns].lengthms=1000.0/(songs[chosenSong].bpm/15.0)*(float)vocals[lns].length;
                //vocals[lns].scoringSegments=(int)(((float)vocals[lns].length*songs[chosenSong].mspb)/scoreGranularity)+0.5;
                vocals[lns].scoringSegments=(int)ceil(vocals[lns].lengthms/scoreGranularity);
                lns++;
            }
            if (temp[0]=='-')
            {
                sscanf(temp,"%c%d",&c,&vocals[lns].beat);
                vocals[lns].type=0;
                vocals[lns].ms=1000.0/(songs[chosenSong].bpm/15.0)*(float)vocals[lns].beat;
                lns++;
            }

        }
    }



}

void loadSongDetails (char * filename,char * path)
{
    FILE * songfile;
    FILE * checkMP3;

    char temp[128];
    char param[128];
    char value[128];
    char edition[128];
    char artist[128];
    char title[128];
    char cover[128];
    char background[128];
    char video[128];
    char mp3[512];
    int fartist=0;
    int ftitle=0;
    int fmp3=0;
    int fvideo=0;
    int fcover=0;
    int songType=1;
    int gap=0;
    float bpm=90.0;

    char * colon;
    int colpos;

    strcpy(edition,"");
    strcpy(cover,"");
    strcpy(background,"");
    strcpy(video,"");

    songfile=fopen (filename,"r");
    if (songfile!=NULL)
    {
        int z;
        while (!(feof(songfile)))
        {
            fgets (temp , 128 , songfile);
            if (temp-strchr(temp,'#')+1==1)
            {
                colon=strchr(temp,':');
                if (colon!=NULL)
                {
                    colpos=colon-temp+1;
                    strncpy(param,temp,colpos);
                    param[colpos]=0;
                    strncpy(value,colon+1,strlen(temp)-(colpos+1));
                    value[strlen(temp)-(colpos+1)]=0;
                    if (value[strlen(temp)-(colpos+2)]==13) value[strlen(temp)-(colpos+2)]=0;
                    if (strcmp(param,"#ARTIST:")==0) {strcpy(artist,value); fartist=1;}
                    if (strcmp(param,"#TITLE:")==0) {strcpy(title,value); ftitle=1;}
                    if (strcmp(param,"#EDITION:")==0) strcpy(edition,value);
                    if (strcmp(param,"#COVER:")==0) {strcpy(cover,value);fcover=1;}
                    if (strcmp(param,"#BACKGROUND:")==0) strcpy(background,value);
                    if (strcmp(param,"#VIDEO:")==0) {strcpy(video,value); fvideo=1;}

                    if (strcmp(param,"#GAP:")==0)
                    {
                        sscanf(value,"%d",&gap);
                    }

                    if (strcmp(param,"#BPM:")==0)
                    {
                        sscanf(value,"%f",&bpm);
                    }

                    if (strcmp(param,"#MP3:")==0)
                    {
                        strcpy(mp3,path);
                        strcat(mp3,"/");
                        strcat(mp3,value);
                        checkMP3=fopen(mp3,"r");
                        if (checkMP3)
                        {
                            fmp3=1;
                        }
                        strcpy(mp3,value);

                    }


                }
            }
        }
        if ((fartist) && (ftitle) && (fmp3))
        {

            songs[numSongs].mp3found=1;
            if (fcover) songs[numSongs].coverfound=1; else songs[numSongs].coverfound=0;
            if (fvideo) songs[numSongs].videofound=1; else songs[numSongs].videofound=0;
            songs[numSongs].artist=(char*)malloc(strlen(artist)+1);
            strcpy(songs[numSongs].artist,artist);
            songs[numSongs].title=(char*)malloc(strlen(title)+1);
            strcpy(songs[numSongs].title,title);
            songs[numSongs].mp3=(char*)malloc(strlen(mp3)+1);
            strcpy(songs[numSongs].mp3,mp3);
            songs[numSongs].edition=(char*)malloc(strlen(edition)+1);
            strcpy(songs[numSongs].edition,edition);
            songs[numSongs].path=(char*)malloc(strlen(path)+1);
            strcpy(songs[numSongs].path,path);
            songs[numSongs].cover=(char*)malloc(strlen(cover)+1);
            strcpy(songs[numSongs].cover,cover);
            songs[numSongs].background=(char*)malloc(strlen(background)+1);
            strcpy(songs[numSongs].background,background);
            songs[numSongs].texture=nocoverTexture;
            songs[numSongs].textureloaded=0;
            songs[numSongs].textfile=(char*)malloc(strlen(filename)+1);
            strcpy(songs[numSongs].textfile,filename);
            songs[numSongs].songtype=songType;

            songs[numSongs].gap=gap;
            songs[numSongs].bpm=bpm;
            //songs[numSongs].mspb=1000.0/(60.0/bpm);
            songs[numSongs].mspb=1000.0/(bpm/60);

            songs[numSongs].scorePerSegment=1;//Needs to be calculated properly

            numSongs++;

        }
    }
    fclose(songfile);
}

int parseSongs()
{
int z,zz;
int numdirs;
char tempstring [512];
char tempstring2 [512];

numSongs=0;

DIR *dp;
struct dirent *ep;

DIR *dp2;
struct dirent *ep2;

//int notdir=0;


    dp=opendir ("./songs");

	if(dp==NULL) return 1;


	numdirs=0;

    while ((ep = readdir (dp)))
    {

        strcpy(&tempstring[0],"./songs/");
        strcat(&tempstring[0],&ep->d_name[0]);
        if (file_length(&tempstring[0])<1)
        {
            // Looks like a directory - check it for song files.
            dp2=opendir(&tempstring[0]);
            if(dp2) numdirs++;

        }

    }

    songs=new songDetails[numdirs]; //Make sure we have enough room if all directories contain a song
    rewinddir(dp);
    while ((ep = readdir (dp)))
    {
        glClearColor(1.0, 1.0, 1.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);

        putFont(&fontGlyph[1],0.0,50.0,12.0,16.0,(char*)"Searching For Songs...",1,1,1);

        strcpy(&tempstring[0],"./songs/");
        strcat(&tempstring[0],&ep->d_name[0]);
        if (file_length(&tempstring[0])<1)
        {
            // Looks like a directory - check it for song files.
           // printf("Opening %s\n",tempstring);
            dp2=opendir(&tempstring[0]);
            if(dp2)
            {
                //rewinddir(dp2);
 //               numdirs++;
                while ((ep2=readdir(dp2)))
                {
                    if (strstr(&ep2->d_name[0],".txt"))
                    {
                        strcpy(&tempstring2[0],&tempstring[0]);
                        strcat(&tempstring2[0],"/");
                        strcat(&tempstring2[0],&ep2->d_name[0]);
                        loadSongDetails(&tempstring2[0],&tempstring[0]);

                    }
                }
            }

        }

        nitoa(numSongs,textbuffer,6);
        putFont(&fontGlyph[1],0.0,70.0,12.0,16.0,(char*)"Songs Found:",1,1,1);
        putFont(&fontGlyph[1],250.0,70.0,12.0,16.0,textbuffer,1,1,1);

        #if defined (HAVE_GLES)
        EGL_SwapBuffers();
        #else
        SDL_GL_SwapBuffers(); //HW DB waits for vertical blank.
        #endif
    }

    return 0;


}

GLuint SurfacetoTexture(char * surffilename)
{
    GLuint texture;
    GLenum texture_format;
    GLint  nOfColors;
    SDL_Surface * surface;

    if ( (surface = IMG_Load(surffilename)) )
    {
        // Check that the image's width is a power of 2
        if ( (surface->w & (surface->w - 1)) != 0 )
        {
            printf("warning: image.bmp's width is not a power of 2\n");
        }

        // Also check if the height is a power of 2
        if ( (surface->h & (surface->h - 1)) != 0 )
        {
		printf("warning: image.bmp's height is not a power of 2\n");
        }

        // get the number of channels in the SDL surface
        nOfColors = surface->format->BytesPerPixel;
        if (nOfColors == 4)     // contains an alpha channel
        {
                if (surface->format->Rmask == 0x000000ff)
                        texture_format = GL_RGBA;
                else
                        //texture_format = GL_BGRA;
                       /* SDL_LockSurface(surface);
                        char * pptr=(char*)surface->pixels;
                        char tc;
                        for (int i=0;i<surface->h;i++)
                            for (int j=0;j<surface->w;j++)
                            {
                                tc=*pptr;
                                *pptr=*(pptr+2);
                                *(pptr+2)=tc;
                                pptr+=4;
                            }
                        SDL_UnlockSurface(surface);*/
                        texture_format = GL_RGBA;
        }
        else if (nOfColors == 3)     // no alpha channel
        {
                if (surface->format->Rmask == 0x000000ff)
                        texture_format = GL_RGB;
                else
                        //texture_format = GL_BGR;
/*                        SDL_LockSurface(surface);
                        char * pptr=(char*)surface->pixels;
                        char tc;
                        for (int i=0;i<surface->h;i++)
                            for (int j=0;j<surface->w;j++)
                            {
                                tc=*pptr;
                                *pptr=*(pptr+2);
                                *(pptr+2)=tc;
                                pptr+=3;
                            }
                        SDL_UnlockSurface(surface);*/
                        texture_format = GL_RGB;
        }
        else
        {
                printf("warning: the image is not truecolor..  this will probably break\n");
                // this error should not go unhandled
        }

        // Have OpenGL generate a texture object handle for us
        glGenTextures( 1, &texture );

        // Bind the texture object
        glBindTexture( GL_TEXTURE_2D, texture );

        // Set the texture's stretching properties
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

        // Edit the texture object's image data using the information SDL_Surface gives us
        glTexImage2D( GL_TEXTURE_2D, 0, texture_format, surface->w, surface->h, 0,texture_format, GL_UNSIGNED_BYTE, surface->pixels );

        if ( surface ) {
            SDL_FreeSurface( surface );
        }

        return texture;

	/*glPixelStorei(GL_UNPACK_ALIGNMENT,4);

	glGenTextures(1,&texture);
	glBindTexture(GL_TEXTURE_2D,texture);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	SDL_PixelFormat *fmt = Surface->format;

    //if there is alpha
	if (fmt->Amask)
	{
		gluBuild2DMipmaps(GL_TEXTURE_2D,4,Surface->w,Surface->h,GL_BGRA,GL_UNSIGNED_BYTE,Surface->pixels);
	}
	else // no alpha
	{
		gluBuild2DMipmaps(GL_TEXTURE_2D,3,Surface->w,Surface->h,GL_BGR,GL_UNSIGNED_BYTE,Surface->pixels);
	}

	SDL_FreeSurface(Surface);

	return texture;*/

    }
    else
    {
        printf("SDL could not load %s: %s\n" ,surffilename, SDL_GetError());
        return 0;
    }

}

void putTexture(GLuint texture,float x,float y,float w,float h, float r, float g, float b)
{

    glBindTexture( GL_TEXTURE_2D, texture );

    glColor3f(r,g,b);

    #if !defined (HAVE_GLES)
        glBegin( GL_QUADS );
        //Bottom-left vertex (corner)
        glTexCoord2f( 0.0, 0.0 );
        glVertex3f( x, y, 0.0f );

        //Bottom-right vertex (corner)
        glTexCoord2f( 1.0, 0.0 );
        glVertex3f( x+w, y, 0.0f );

        //Top-right vertex (corner)
        glTexCoord2f( 1.0, 1.0 );
        glVertex3f( x+w, y+h, 0.0f );

        //Top-left vertex (corner)
        glTexCoord2f( 0.0, 1.0 );
        glVertex3f( x, y+h, 0.0f );
        glEnd();
        #else
        GLfloat vtx1[]={
            x,y,0,
            x+w,y,0,
            x+w,y+h,0,
            x,y+h,0};
        GLfloat tex1[]={
            0,0,
            1,0,
            1,1,
            0,1};
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);

        glVertexPointer(3,GL_FLOAT,0,vtx1);
        glTexCoordPointer(2,GL_FLOAT,0,tex1);
        glDrawArrays(GL_TRIANGLE_FAN,0,4);

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        #endif
}

bool ProcessEvents()
{
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
 //   case SDL_QUIT:
 //     return false;
    case SDL_VIDEORESIZE:
      printf("video resize\n");
      glViewport(0, 0, event.resize.w, event.resize.h);
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      break;
    default:
      break;
    }
  }
  return true;
}

float putFont(sGlyph *fGlyph,float x,float y,float w,float h,char * t,float r, float g, float b)
{
    if (!(t)) return 0.0;
    glBindTexture( GL_TEXTURE_2D, fGlyph->texture );

    glColor3f(r,g,b);

    GLfloat dx=1.0/(float)fGlyph->lettersAcross;
    GLfloat dy=1.0/(float)fGlyph->lettersDown;
    GLfloat dx2=dx-0.002;
    GLfloat dy2=dy-0.002;

    unsigned int z,c,px,py;

    for (z=0;z<strlen(t);z++)
    {

    c=*(t+z);

    py=c/fGlyph->lettersAcross;
    px=c-(py*fGlyph->lettersAcross);

  //  printf("char:%d px:%d py:%d\n",c,px,py);
    #if !defined (HAVE_GLES)
    glBegin( GL_QUADS );
        //Bottom-left vertex (corner)
        glTexCoord2f( (float)px*dx,(float)py*dy );
        glVertex3f( x+(z*w), y, 0.0f );

        //Bottom-right vertex (corner)
        glTexCoord2f( (float)px*dx+dx2,(float)py*dy );
        glVertex3f( x+w+(z*w), y, 0.0f );

        //Top-right vertex (corner)
        glTexCoord2f( (float)px*dx+dx2, (float)py*dy+dy2 );
        glVertex3f( x+w+(z*w), y+h, 0.0f );

        //Top-left vertex (corner)
        glTexCoord2f( (float)px*dx, (float)py*dy+dy2 );
        glVertex3f( x+(z*w), y+h, 0.0f );
    glEnd();
    #else
        GLfloat vtx1[]={
            x+(z*w), y, 0.0f,
            x+w+(z*w), y, 0.0f ,
            x+w+(z*w), y+h, 0.0f,
            x+(z*w), y+h, 0.0f};
        GLfloat tex1[]={
            (float)px*dx,(float)py*dy,
            (float)px*dx+dx2,(float)py*dy,
            (float)px*dx+dx2, (float)py*dy+dy2 ,
            (float)px*dx, (float)py*dy+dy2};
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);

        glVertexPointer(3,GL_FLOAT,0,vtx1);
        glTexCoordPointer(2,GL_FLOAT,0,tex1);
        glDrawArrays(GL_TRIANGLE_FAN,0,4);

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        #endif


    }

    return strlen(t)*w;
}

int displaySongs()
{
    int z;
    float x,y;
    int keydata=0;
    float sx,sy;
    int currentSong=0;
    int texturemanager=0;

    char temp[17];

    int xr;

    sx=0.0;
    sy=0.0;

    glPushMatrix();

    glTranslatef(256,128,0);
    glTranslatef(0,0,0);

    while (!(keydata&MY_BUTT_B))
    {
        x=0;y=0;xr=0;

      //  glOrtho(sx, SCREEN_WIDTH+sx, SCREEN_HEIGHT+sy, sy, -1.0f, 1.0f);
    //    glMatrixMode( GL_MODELVIEW );
      //  glLoadIdentity();
        glClearColor(1.0,1.0, 1.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);
        z=0;

    //    printf("%d\n",songs[texturemanager].coverfound);

        if (texturemanager<numSongs)
        {
            if ((songs[texturemanager].textureloaded==0) && (songs[texturemanager].coverfound==1))
            {
                char cs[512];
                strcpy(cs,songs[texturemanager].path);
                strcat(cs,"/");
                strcat(cs,songs[texturemanager].cover);
                printf("Loading cover from path %s\n",cs);
                songs[texturemanager].texture=SurfacetoTexture(cs);
                if (songs[texturemanager].texture==0) songs[texturemanager].texture=nocoverTexture; else songs[texturemanager].textureloaded=1;

            }
            texturemanager++;
            if (texturemanager>numSongs) texturemanager=0;
        }

        while (z<numSongs)
        {
           glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            if (z==currentSong)
            {
                putTexture(songs[z].texture,x-36,y-36,200,200,1,1,1);
                putTexture(borderTexture,x-86,y+180,300,128,1,1,1);
                putTexture(musiciconTexture,x+170,y+262,32,32,1,1,1);
                if (songs[z].videofound) putTexture(videoiconTexture,x+130,y+262,32,32,1,1,1);
                glBlendFunc(GL_SRC_ALPHA,GL_ZERO);
                putFont(&fontGlyph[1],x-82,y+184,20,24,songs[z].title,1,1,1);
                putFont(&fontGlyph[1],x-82,y+208,20,24,songs[z].artist,1,1,1);
            }
            else
            {
                putTexture(songs[z].texture,x,y,128,128,1,1,1);
                glBlendFunc(GL_SRC_ALPHA,GL_ZERO);

                if (strlen(songs[z].title)>16)
                {
                    strncpy(temp,songs[z].title,13);
                    temp[13]='.';
                    temp[14]='.';
                    temp[15]='.';
                    temp[16]=0;
                }
                else
                {
                    strcpy(temp,songs[z].title);
                }
                putFont(&fontGlyph[1],x,y+128,12,16,temp,1,1,1);
                if (strlen(songs[z].artist)>16)
                {
                    strncpy(temp,songs[z].artist,13);
                    temp[13]='.';
                    temp[14]='.';
                    temp[15]='.';
                    temp[16]=0;
                }
                else
                {
                    strcpy(temp,songs[z].artist);
                }
                putFont(&fontGlyph[1],x,y+144,12,16,temp,1,1,1);
            }

            //glEnable(GL_BLEND);
        //    printf("Artist: %s, Title: %s\n",songs[z].artist,songs[z].title);

            x=x+256;
            xr++;
            if (xr>=10) {x=0;y=y+256;xr=0;}
            z++;
        }
        glFlush();
        #if defined (HAVE_GLES)
        EGL_SwapBuffers();
        #else
        SDL_GL_SwapBuffers(); //HW DB waits for vertical blank.
        #endif
        SDL_Delay(100);
        keydata=get_key();
        glTranslatef(0,0,0);
        if (keydata & MY_RIGHT) {glTranslatef(-256,0,0);currentSong++;}
        if (keydata & MY_LEFT) {glTranslatef(256,0,0);currentSong--;}
        if (keydata & MY_UP) {glTranslatef(0,256,0);currentSong-=10;}
        if (keydata & MY_DOWN) {glTranslatef(0,-256,0);currentSong+=10;}
    }
    glPopMatrix();
    return currentSong;
}



int main( int argc, char* args[] )
{

    int err,z;



//    pGlyphSet = ReadFontFile("Eurostile.xml");


    #ifdef _WIN32
    _setmode( _fileno( stdin ), _O_BINARY );
    _setmode( _fileno( stdout ), _O_BINARY );
    #endif


    set_new_handler(no_memory);

    // Initialize all SDL subsystems
    err=SDL_Init( SDL_INIT_JOYSTICK | SDL_INIT_VIDEO | SDL_INIT_TIMER );

    if (err ==-1)
    {
        printf(  "SDL Init error: %d\n",  err  );
        return 1;
    }
    #if defined (HAVE_GLES)
    if (EGL_Open()) exit(1);
    #endif


    SetOpenGLAtributes();

    // Create the screen surface
    #if defined (HAVE_GLES)
    screen = SDL_SetVideoMode( 800, 480, 0, SDL_SWSURFACE | SDL_FULLSCREEN );
    EGL_Init();
    #else
    screen = SDL_SetVideoMode( SCREEN_WIDTH, SCREEN_HEIGHT, 16, SDL_HWSURFACE | SDL_HWPALETTE | SDL_OPENGL | SDL_NOFRAME );
    #endif


    if (!(BASS_Init(-1,44100,0,0,NULL)))
    {
        printf("BASS Library Initialisation Failed\n");
        printf("BASS Error was %d\n",BASS_ErrorGetCode());
        return 1;
    }
    else printf("BASS Library Initiated Successfully\n");

 //   SDL_Event resizeEvent;
  //  resizeEvent.type = SDL_VIDEORESIZE;
  //  resizeEvent.resize.w = 800;
  //  resizeEvent.resize.h = 480;
 //   SDL_PushEvent(&resizeEvent);

    //**** Open GL Setup ****
    glEnable( GL_TEXTURE_2D );

    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );

    glViewport( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT );

    glClear( GL_COLOR_BUFFER_BIT );

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();

   // glBlendFunc(GL_SRC_ALPHA, GL_ONE);
   // glBlendFunc(GL_DST_COLOR, GL_ZERO);
    //glBlendFunc(GL_ZERO, GL_ZERO);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_BLEND);


    glOrtho(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, -1.0f, 1.0f);

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    //****

   // LoadTexture("font.png", &texName);
  //glEnable(GL_TEXTURE_2D);

  /*  SDL_Surface* img_temp;
    img_temp = IMG_Load("myfont.png");
	if(!img_temp)
	{
	    err=1;
		printf("resource myfont.png not found.\n");								// debug output example for serial cable
	}
	else myfont = SDL_ConvertSurface(img_temp, screen->format, SDL_HWSURFACE);
	SDL_FreeSurface(img_temp);*/



	// Do PC specific stuff
#ifdef _WIN32
	// Set the window caption
	SDL_WM_SetCaption( "Singstar", NULL );
#endif // PLATFORM_PC


    int numDevices=0;
    int a=0;
    BASS_DEVICEINFO info;
    for (a=0;BASS_RecordGetDeviceInfo(a,&info);a++)
        if (info.flags&BASS_DEVICE_ENABLED) numDevices++;


    BASS_DEVICEINFO deviceInfo[numDevices];
    if( numDevices <= 0 )
    {
        printf( "ERROR: BASS returned 0 devices\n");
        err = numDevices;
        return 3;
    }
    else
    {
        //const PaDeviceInfo *deviceInfo;
        printf( "SUCCESS: BASS returned 0x%x devices\n", numDevices );
        for (z=0;z<numDevices;z++)
        {
            BASS_RecordGetDeviceInfo(z,&deviceInfo[z]);
            //deviceDetails[z].input=deviceInfo->maxInputChannels;
            //strncpy(&deviceDetails[z].name[0],deviceInfo->name,50);
            //if (deviceInfo[z]->maxInputChannels>0) printf("Device %d (%s) as %d input channels\n",z,deviceInfo[z]->name,deviceInfo[z]->maxInputChannels);
        }

    }

    //SDL_Rect fullscreen = {0,0,SCREEN_WIDTH,SCREEN_HEIGHT};

    nocoverTexture=SurfacetoTexture((char*)"nocover.jpg");
    borderTexture=SurfacetoTexture((char*)"border.bmp");
    musiciconTexture=SurfacetoTexture((char*)"record.bmp");
    videoiconTexture=SurfacetoTexture((char*)"video.bmp");
    leftbarTexture=SurfacetoTexture((char*)"leftbar.bmp");
    midbarTexture=SurfacetoTexture((char*)"midbar.bmp");
    rightbarTexture=SurfacetoTexture((char*)"rightbar.bmp");
    arrowsTexture=SurfacetoTexture((char*)"arrows.bmp");

    //fontGlyph.texture=SurfacetoTexture("data/fonts/franks.png");
    fontGlyph[0].texture=SurfacetoTexture((char*)"data/fonts/abstract-b.bmp");
    fontGlyph[0].width=512;
    fontGlyph[0].height=512;
    fontGlyph[0].lettersAcross=16;
    fontGlyph[0].lettersDown=16;

    fontGlyph[1].texture=SurfacetoTexture((char*)"data/fonts/arial.bmp");
    fontGlyph[1].width=512;
    fontGlyph[1].height=512;
    fontGlyph[1].lettersAcross=16;
    fontGlyph[1].lettersDown=16;

    parseSongs();
    printf("%d Song(s) Found\n",numSongs);

    int keydata=0;
    int selectedDevice=0;
    while (keydata!=MY_BUTT_A)
    {


        //SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0,0,0));
        glClearColor(1.0, 1.0, 1.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);

        //putTexture(lisatexture,100.0,100.0,128.0,128.0);
        //putTexture(lisatexture,400.0,300.0,128.0,128.0);

        //putFont(fontGlyph,0.0,0.0,16.0,16.0,"TEST");



        putFont(&fontGlyph[0],0.0,30.0,32.0,32.0,(char*)"SELECT INPUT DEVICE",1,1,1);
        //put_text(0,80,&deviceDetails[selectedDevice].name[0],29,0);
        putFont(&fontGlyph[1],0.0,80.0,12.0,16.0,(char*)deviceInfo[selectedDevice].name,1,1,1);
        //if (deviceInfo[selectedDevice]->maxInputChannels<1) putFont(&fontGlyph[1],0.0,120.0,16.0,16.0,(char*)"NO INPUT",1,1,1);
        //myInfo=Pa_GetHostApiInfo(deviceInfo[selectedDevice]->hostApi);
        putFont(&fontGlyph[1],0.0,100.0,12.0,16.0,(char*)deviceInfo[selectedDevice].driver,1,1,1);

        #if defined (HAVE_GLES)
        EGL_SwapBuffers();
        #else
        SDL_GL_SwapBuffers(); //HW DB waits for vertical blank.
        #endif

        SDL_Delay(100);
        keydata=get_key();
        if ((keydata==MY_RIGHT) && (selectedDevice<numDevices-1)) selectedDevice++;
        if ((keydata==MY_LEFT) && (selectedDevice>0)) selectedDevice--;

    }
    printf("Left first loop\n");

    //**** Display Song Selection Screen
    chosenSong=displaySongs();



    //**** Allocate enough space for 4096 16bit stereo samples (4096 * 2 * 2)
    myData=(float *) malloc((REC_BUFFER*2)<<5);

    //**** Open Input Stream
    BASS_RecordInit(selectedDevice);
    BASS_RecordGetInfo(&recordinfo);
    recordchannels=recordinfo.inputs;
    printf("Inputs for selected device=%d\n",recordchannels);
    //goto forcequit;
    if (recordchannels<1) goto forcequit;
    micStream=BASS_RecordStart(44100,2,MAKELONG(BASS_RECORD_PAUSE,20),&micCallback,myData);
    //err=Pa_OpenStream(&micStream,&micInput,NULL,44100,REC_BUFFER*2,paNoFlag,micCallback,myData);
 //   printf(  "PortAudio Openstream Message: %s\n", Pa_GetErrorText( err ) );

    //**** Create music file load string and load
    char tempfile[512];
    strcpy(tempfile,songs[chosenSong].path);
    strcat(tempfile,"/");
    strcat(tempfile,songs[chosenSong].mp3);

    printf ("Chosen Song No: %d - %s\n",chosenSong,songs[chosenSong].title);
    printf ("Attempting to load music file: %s\n",tempfile);

 /*   music = Mix_LoadMUS(tempfile);
    if(music == NULL)
    {
        printf("Unable to load music file: %s\n", Mix_GetError());
        return 1;
    }
    else printf("Music file loaded ok\n");*/

    music=BASS_StreamCreateFile(false,tempfile,0,0,0);
    if (music==0)
    {
        int err=BASS_ErrorGetCode();
        printf("Music Load Failed - BASS Error Code %d\n",err);

    }
    else
    {
        printf("Music file loaded ok\n");
        BASS_ChannelUpdate(music,0);
    }


 /*   //**** Load text file into memory
  //  strcpy(tempfile,"");
    strcpy(tempfile,songs[chosenSong].textfile);
 //   strcat(tempfile,"/");
 //   strcat(tempfile,songs[chosenSong].textfile);

    printf ("Attempting to load text file: %s\n",tempfile);

    int bytesNeeded=file_length(tempfile);
    if (bytesNeeded>0)
    {
        FILE * textfileHandle;
        int bytesRead;
        songs[chosenSong].song=(char*) malloc(bytesNeeded);
        textfileHandle=fopen(tempfile,"r+b");
        bytesRead=fread(songs[chosenSong].song,1,bytesNeeded,textfileHandle);
        fclose(textfileHandle);
        textfileHandle=NULL;
        if (bytesRead==bytesNeeded) printf("Song text loaded successfully\n"); else printf ("Song Text Warning: %d bytes expected but %d bytes loaded\n",bytesNeeded,bytesRead);
    }
    else
    {
        printf ("Failed to load song text - could not calculate file size\n");
    }*/

    playerDetails=new player[2]; //Create player structures
    playerDetails[0].inputChannel=0;
    playerDetails[1].inputChannel=1;


    switch (songs[chosenSong].songtype)
    {
        default:
            parseUltrastar(songs[chosenSong].textfile);
            break;
    }


    //**** Pre-calculate Hann Windowfor speed purposes
    createHannWindow();

    //**** Routine to sing selected song
    err=call_song();

    BASS_StreamFree(music);
    //free(songs[chosenSong].song);


    //***** Closedown Code
    delete[] vocals;
    delete[] songs;
    delete[] playerDetails;

    forcequit:

    BASS_RecordFree();
    BASS_Free();


    #if defined (HAVE_GLES)
    EGL_Close();
    #endif
    SDL_Quit();

    // Do GP2X specific stuff
#ifdef PLATFORM_GP2X
    // Return to the menu
    chdir("/usr/gp2x");
	execl("/usr/gp2x/gp2xmenu", "/usr/gp2x/gp2xmenu", NULL);
#endif // PLATFORM_GP2X

    return 0;
}

