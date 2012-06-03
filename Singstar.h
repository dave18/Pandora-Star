
#define GP2X_BUTTON_UP              (0)
#define GP2X_BUTTON_DOWN            (4)
#define GP2X_BUTTON_LEFT            (2)
#define GP2X_BUTTON_RIGHT           (6)
#define GP2X_BUTTON_UPLEFT          (1)
#define GP2X_BUTTON_UPRIGHT         (7)
#define GP2X_BUTTON_DOWNLEFT        (3)
#define GP2X_BUTTON_DOWNRIGHT       (5)
#define GP2X_BUTTON_CLICK           (18)
#define GP2X_BUTTON_A               (12)
#define GP2X_BUTTON_B               (13)
#define GP2X_BUTTON_X               (15)
#define GP2X_BUTTON_Y               (14)
#define GP2X_BUTTON_L               (11)
#define GP2X_BUTTON_R               (10)
#define GP2X_BUTTON_START           (8)
#define GP2X_BUTTON_SELECT          (9)
#define GP2X_BUTTON_VOLUP           (16)
#define GP2X_BUTTON_VOLDOWN         (17)

#define MY_LEFT                     (1)
#define MY_RIGHT                     (2)
#define MY_UP                     (4)
#define MY_DOWN                     (8)
#define MY_BUTT_A                   (16)
#define MY_BUTT_B                   (32)
#define MY_BUTT_X                     (64)
#define MY_BUTT_SL                   (128)
#define MY_BUTT_SR                   (256)
#define MY_START                     (512)
#define MY_QT                       (1024)


typedef unsigned char u8;

const float base_a4=440; // set A4=440Hz
const float scoreGranularity=20.0;  //How many milliseconds between scoring segmnents


struct sGlyph
{
    GLfloat width;
    GLfloat height;
    int lettersAcross;
    int lettersDown;
    GLuint texture;
    char * name;
};


int get_key(void);
void createHannWindow(void);
GLuint SurfacetoTexture(char *);
void putTexture(GLuint ,float,float,float,float,float,float,float);
float putFont(sGlyph*,float,float,float,float,char *,float, float, float);

SDL_Surface* screen;
//SDL_Surface* myfont;

//Mix_Music *music;

HSTREAM music;



struct myDevices {
	int input;
	char name[51];
};


struct vocalLines{
    int type;   //0=verse 1=normal 2=freestyle 3=golden note
    float ms;
    int beat;
    int length;
    float lengthms;
    int pitch;
    char * lyrics;
    int scoringSegments;
};

struct results{
    int * recordedPitches;
};

struct player {
    char * name;
    int inputChannel;
    float score;
    //int * recordedPitches;

    int currentPitch;

    results * playerResults;
    //int modeArray[88]; //Used to apply mode average to singing pitch  ** not used anymore as no time to build up array
};

struct songDetails {
    char * title;
    char * artist;
    char * edition;
    char * path;
    char * textfile;
    char * mp3;
    char * video;
    char * cover;
    char * background;
    int songtype; //1 = ultrastar
    int textureloaded;
    GLuint texture;
    int mp3found;
    int videofound;
    int coverfound;
    float bpm;
    int gap;
    int numVocals;
    float mspb;
    float scorePerSegment;
   // void * song;
};

struct inputStruct {
    float x[4096];
    float f[4096];
    float mag[2048];
    float freq;
    float intfreq;
    int highserial;
    float highmag;
    float reducedfreq;
    float intserial;
    int pitch;
    int adjpitch;
};



char const ENDCHAR=0;
char const SPACECHAR=' ';
char const EXCLMCHAR='!';
char const STOPCHAR='.';



// Freq        Ultrastar Pitch
// 110         9
// 220         21
// 440         33


