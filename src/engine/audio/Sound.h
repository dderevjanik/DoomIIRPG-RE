#pragma once
#include <al.h>
#include <alc.h>

typedef uint32_t AudioFormatID;
typedef uint32_t AudioFormatFlags;
typedef uint32_t AudioFileID;
struct AudioStreamBasicDescription
{
	uint32_t mSampleRate;
	AudioFormatID mFormatID;
	AudioFormatFlags mFormatFlags;
	uint32_t mBytesPerPacket;
	uint32_t mFramesPerPacket;
	uint32_t mBytesPerFrame;
	uint32_t mChannelsPerFrame;
	uint32_t mBitsPerChannel;
	uint32_t mReserved;
};

class Applet;


class Sound
{
private:

public:

	class SoundStream
	{
	public:
		ALuint bufferId = 0;
		ALuint sourceId = 0;
		int16_t resID = 0;
		int16_t priority = 0;
		bool fadeInProgress = false;
		uint8_t pad_0xd = 0;
		uint8_t pad_0xe = 0;
		uint8_t pad_0xf = 0;
		int volume = 0;
		int fadeBeg = 0;
		int fadetime = 0;
		float fadeVolume = 0.0f;

		void StartFade(int volume, int fadeBeg, int fadeEnd);
	};

	Applet* app = nullptr;
	bool headless = false;
	bool isMuted = false; // guessed — blocks sound playback when true
	bool allowSounds = false;
	bool allowMusics = false;
	int soundFxVolume = 0;
	int musicVolume = 0;
	int soundCapacity = 0; // guessed — playback blocked when 0, initialized to 100
	SoundStream channel[10] = {};
	short resID = 0;
	uint8_t flags = 0;
	int priority = 0;
	int unused_0x15c = 0; // set to -1 but never read
	short unused_0x160 = 0; // set to -1 but never read
	uint8_t hasDeferredSound = 0; // guessed — triggers deferred playback in startFrame
	bool soundsLoaded = false;
	ALCcontext* alContext = nullptr;
	ALCdevice* alDevice = nullptr;

	// Constructor
	Sound();
	// Destructor
	~Sound();

	bool startup();
	void openAL_Init();
	void openAL_Close();
	void openAL_SetSystemVolume(int volume);
	void openAL_SetVolume(ALuint source, int volume);
	void openAL_Suspend();
	void openAL_Resume();
	bool openAL_IsPlaying(ALuint source);
	bool openAL_IsPaused(ALuint source);
	bool openAL_GetALFormat(AudioStreamBasicDescription aStreamBD, ALenum* format);
	void openAL_PlaySound(ALuint source, ALint loop);
	void openAL_LoadSound(int resID, Sound::SoundStream* channel);
	bool openAL_LoadWAVFromFile(ALuint bufferId, const char* fileName);
	bool openAL_LoadAudioFileData(const char* fileName, ALenum* format, ALvoid** data, ALsizei* size, ALsizei* freq);
	bool openAL_OpenAudioFile(const char* fileName, InputStream* IS);
	bool openAL_LoadAllSounds();

	bool cacheSounds();
	void playSound(int16_t resID, uint8_t flags, int priority, bool a5);
	int getFreeSlot(int a2);
	void soundStop();
	void stopSound(int resID, bool fadeOut);
	bool isSoundPlaying(int16_t resID);
	void updateVolume();
	void playCombatSound(int16_t resID, uint8_t flags, int priority);
	bool cacheCombatSound(int resID);
	void freeMonsterSounds();

	void volumeUp(int volume);
	void volumeDown(int volume);
	void startFrame();
	void endFrame();
	void musicVolumeUp(int volume); // [GEC]
	void musicVolumeDown(int volume); // [GEC]
};
