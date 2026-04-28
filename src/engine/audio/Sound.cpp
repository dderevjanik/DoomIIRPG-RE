#include <cstdlib>
#include <print>
#include <stdexcept>
#include <al.h>
#include <alc.h>
#include "Log.h"

#include "App.h"
#include "CAppContainer.h"
#include "Sound.h"
#include "Sounds.h"
#include "JavaStream.h"

Sound::Sound() = default;

Sound::~Sound() {
	if (this->headless) { return; }
	this->soundStop();
	this->openAL_Close();
}

#define OpenAL_ERROR(id)                                                                                               \
	error = alGetError();                                                                                              \
	if (error != AL_NO_ERROR) {                                                                                        \
		LOG_ERROR("OpenAL error: {}, file: {} [{}]\n", alGetString(error),                                                \
		       "/Users/greghodges/doom2rpg/trunk/Doom2rpg_iphone/xcode/Classes/Sound.cpp", id);                        \
	}

bool Sound::startup() {
	this->app = CAppContainer::getInstance()->app;
	this->headless = CAppContainer::getInstance()->headless;
	LOG_INFO("[sound] startup\n");

	this->hasDeferredSound = 0;
	this->app = app;
	this->allowSounds = true;
	this->allowMusics = true;
	this->soundFxVolume = 80;
	this->musicVolume = 80;
	this->soundCapacity = 100;
	this->isMuted = 0;
	this->unused_0x160 = -1;
	this->soundsLoaded = false;
	this->alContext = nullptr;

	// sounds.yaml is already loaded in loadTables() (before menuSystem needs it)

	for (int i = 0; i < 10; i++) {
		this->channel[i].resID = -1;
		this->channel[i].priority = 1;
	}

	if (!this->headless) {
		this->openAL_Init();
	}

	return true;
}

void Sound::openAL_Init() {
	ALenum error;
	LOG_INFO("[sound] openAL_Init\n");

	alGetError();
	if (alcGetCurrentContext()) {
		std::println(stderr, "WARNING! Trying to init OpenAL, but we already have a context!");
	} else {
		this->alDevice = alcOpenDevice(nullptr);
		OpenAL_ERROR(522);

		if (this->alDevice) {
			ALCcontext* context = alcCreateContext(this->alDevice, nullptr);
			OpenAL_ERROR(527);

			if (context) {
				alcMakeContextCurrent(context);
				this->alContext = context;
			} else {
				alcCloseDevice(this->alDevice);
			}
		}
		OpenAL_ERROR(540);

		// this->openAL_SetSystemVolume(100);
		alListener3i(AL_POSITION, 0, 0, 0);
		alListener3i(AL_VELOCITY, 0, 0, 0);
		OpenAL_ERROR(546);
	}
}

void Sound::openAL_Close() {
	for (int i = 0; i < 10; i++) {
		alDeleteBuffers(1, &this->channel[i].bufferId);
		alDeleteSources(1, &this->channel[i].sourceId);
	}

	alcMakeContextCurrent(nullptr);
	if (this->alContext) {
		alcDestroyContext(this->alContext);
		this->alContext = nullptr;
	}
	if (this->alDevice) {
		alcCloseDevice(this->alDevice);
		this->alDevice = nullptr;
	}
}

void Sound::openAL_SetSystemVolume(int volume) {
	if (volume > 100) {
		volume = 100;
	}
	alListenerf(AL_GAIN, (float)volume / 100.0f);
}

void Sound::openAL_SetVolume(ALuint source, int volume) {
	if (volume > 100) {
		volume = 100;
	}
	alSourcef(source, AL_MAX_GAIN, (float)volume / 100.0);
}

void Sound::openAL_Suspend() {
	if (this->alContext) {
		alcMakeContextCurrent(nullptr);
		alcSuspendContext(this->alContext);
	}
}

void Sound::openAL_Resume() {
	if (this->alContext) {
		alcMakeContextCurrent(this->alContext);
		alcProcessContext(this->alContext);
	}
}

bool Sound::openAL_IsPlaying(ALuint source) {
	ALenum error;
	ALint source_state;
	alGetSourcei(source, AL_SOURCE_STATE, &source_state);
	OpenAL_ERROR(685);
	return (source_state == AL_PLAYING) ? true : false;
}

bool Sound::openAL_IsPaused(ALuint source) {
	ALenum error;
	ALint source_state;
	alGetSourcei(source, AL_SOURCE_STATE, &source_state);
	OpenAL_ERROR(694);
	return (source_state == AL_PAUSED) ? true : false;
}

bool Sound::openAL_GetALFormat(AudioStreamBasicDescription aStreamBD, ALenum* format) {
	if ((aStreamBD.mFormatID == 'lpcm') && (aStreamBD.mChannelsPerFrame - 1U < 2)) {
		if (aStreamBD.mBitsPerChannel == 8) {
			if (aStreamBD.mChannelsPerFrame == 1) {
				*format = AL_FORMAT_MONO8;
			} else {
				*format = AL_FORMAT_STEREO8;
			}
			return true;
		}
		if (aStreamBD.mBitsPerChannel == 16) {
			if (aStreamBD.mChannelsPerFrame == 1) {
				*format = AL_FORMAT_MONO16;
			} else {
				*format = AL_FORMAT_STEREO16;
			}
			return true;
		}
	}
	return false;
}

void Sound::openAL_PlaySound(ALuint source, ALint loop) {
	ALenum error;
	alSourcei(source, AL_LOOPING, (loop != 0) ? AL_TRUE : AL_FALSE);
	OpenAL_ERROR(708);
	alSourcePlay(source);
	OpenAL_ERROR(711);
}

void Sound::openAL_LoadSound(int resID, Sound::SoundStream* channel) {
	ALenum error;
	LOG_INFO("[sound] openAL_LoadSound: resID={} buffer={}\n", resID, channel->bufferId);
	int index = (uint16_t)(resID - 1000);
	OpenAL_ERROR(596);

	const char* fileName = Sounds::getFileName(index);
	if (!fileName) {
		this->app->Error("Sound index %d out of range\n!", index);
		return;
	}

	LOG_INFO("[sound] Loading sound: name={} resID={} buffer={}\n", fileName, resID, channel->bufferId);
	if (this->openAL_LoadWAVFromFile(channel->bufferId, fileName)) {
		OpenAL_ERROR(606);
	} else {
		this->app->Error("Sound resource not found\n!");
	}
}

bool Sound::openAL_LoadWAVFromFile(ALuint bufferId, const char* fileName) {
	ALenum error;
	ALsizei freq;
	ALvoid* data;
	ALsizei size;
	ALenum format;

	data = nullptr;
	LOG_INFO("[sound] Loading wav: {}\n", fileName);
	if (this->openAL_LoadAudioFileData(fileName, &format, &data, &size, &freq)) {
		alBufferData(bufferId, format, data, size, freq);
		std::free(data);
		OpenAL_ERROR(939);
		return true;
	}
	return false;
}

bool Sound::openAL_LoadAudioFileData(const char* fileName, ALenum* format, ALvoid** data, ALsizei* size,
                                     ALsizei* freq) {
	InputStream IS;
	AudioStreamBasicDescription outPropertyData;
	char buffer[4];

	if (this->openAL_OpenAudioFile(fileName, &IS)) {
		IS.read((uint8_t*)buffer, 0, 4);
		if (strncmp(buffer, "RIFF", 4) != 0) {
			this->app->Error("Not a valid WAV file, RIFF not found in header\n!");
		}

		IS.readInt(); // size of file

		IS.read((uint8_t*)buffer, 0, 4);
		if (strncmp(buffer, "WAVE", 4) != 0) {
			this->app->Error("Not a valid WAV file, WAVE not found in header\n!");
		}

		IS.read((uint8_t*)buffer, 0, 4);
		if (strncmp(buffer, "fmt ", 4) != 0) {
			this->app->Error("Not a valid WAV file, Format Marker not found in header\n!");
		}

		int chunklength = IS.readInt();
		if ((chunklength != 16) && (chunklength != 18)) {
			this->app->Error("Not a valid WAV file, format length wrong in header\n!");
		}

		if (IS.readShort() != 1) {
			this->app->Error("Not a valid WAV file, file not in PCM format\n!");
		}

		outPropertyData.mFormatID = 'lpcm';
		outPropertyData.mChannelsPerFrame = IS.readShort(); // Get number of channels.
		outPropertyData.mSampleRate = IS.readInt();         // Get sampler rate.
		IS.readInt();                                       // Block Align
		IS.readShort();                                     // ByteRate
		outPropertyData.mBitsPerChannel = IS.readShort();   // Get Bits Per Sample.
		if (chunklength == 18) {                            // SpiderMastermind_sight.wav use chunklength 18
			IS.readShort();
		}
		IS.readInt();         // "data" chunk.
		*size = IS.readInt(); // Get size of the data.

		*data = (ALvoid*)std::malloc(*size);
		IS.read((uint8_t*)*data, 0, *size); // Read audio data into buffer.

		IS.close();
		IS.~InputStream();

		if (!this->openAL_GetALFormat(outPropertyData, format)) {
			this->app->Error("openAL: Error formatting");
		}

		*freq = (int)outPropertyData.mSampleRate;
		return true;
	}
	IS.~InputStream();
	return false;
}

bool Sound::openAL_OpenAudioFile(const char* fileName, InputStream* IS) {
	if (!IS->loadFile(fileName, LT_SOUND_RESOURCE)) {
		this->app->Error("Failed to open audio: %s", fileName);
		return false;
	}

	return true;
}

bool Sound::openAL_LoadAllSounds() {
	ALenum error;
	if (!this->soundsLoaded) {
		for (int i = 0; i < 10; i++) {
			alGenBuffers(1, &this->channel[i].bufferId);
			alGenSources(1, &this->channel[i].sourceId);
			alSource3i(this->channel[i].sourceId, AL_POSITION, 0, 0, 0);
			alSourcei(this->channel[i].sourceId, AL_REFERENCE_DISTANCE, 5000000);
		}
		OpenAL_ERROR(643);
		this->soundsLoaded = true;
		return true;
	}

	return false;
}

bool Sound::cacheSounds() {
	if (this->headless) { return true; }
	if (this->openAL_LoadAllSounds()) {
		this->updateVolume();
		return true;
	}
	return false;
}

void Sound::playSound(int16_t resID, uint8_t flags, int priority, bool unused) {
	if (this->headless) { return; }
	ALenum error;

	int soundResID;
	bool isInvalid;
	bool isBlocked;
	int existingSlot;
	int chanIdx;
	int stopIdx;
	int volume;
	int freeSlot;

	SoundStream* channel;

	soundResID = resID;
	isInvalid = resID == -1;
	if (resID != -1)
		isInvalid = resID == 255;

	if (!isInvalid) {
		isBlocked = resID == 1255;
		if (resID != 1255)
			isBlocked = this->soundCapacity == 0;
		if (!isBlocked && !this->isMuted) {
			if ((unsigned int)(resID - 1067) <= 4) {
				if (!this->allowMusics /* || isUserMusicOn()*/)
					return;
			} else if (!this->allowSounds) {
				return;
			}
			if (this->resID == soundResID)
				return;
			freeSlot = -1;
			for (chanIdx = 0; chanIdx < 10; chanIdx++) {
				if (this->channel[chanIdx].resID == soundResID) {
					if (!this->openAL_IsPlaying(this->channel[chanIdx].sourceId)) {
						existingSlot = chanIdx;
						goto check_play_mode;
					}
					if (priority == 6) {
						existingSlot = chanIdx;
						return;
					}
				}

				if (freeSlot == -1 && this->channel[chanIdx].resID == -1) {
					freeSlot = chanIdx;
				}
			}

			existingSlot = -1;
		check_play_mode:
			if (priority == 6 || (flags & 1) != 0) {
			skip_if_playing:
				if (existingSlot != -1 && this->openAL_IsPlaying(this->channel[existingSlot].sourceId))
					return;
			}
			if ((flags & 2) != 0) {
				for (stopIdx = 0; stopIdx < 10; stopIdx++) {
					alSourceStop(this->channel[stopIdx].sourceId);
					this->channel[stopIdx].priority = 1;
					this->channel[stopIdx].fadeInProgress = 0;
				}
			}

			if (existingSlot == -1) {
				if (freeSlot == -1) {
					freeSlot = this->getFreeSlot(priority);
					if (freeSlot == -1)
						return;
				}
				channel = &this->channel[freeSlot];
				channel->resID = resID;
				channel->priority = priority;
				alSourceStop(channel->sourceId);
				alSourcei(channel->sourceId, AL_BUFFER, 0);
				this->openAL_LoadSound(soundResID, &this->channel[freeSlot]);
				alSourcei(channel->sourceId, AL_BUFFER, channel->bufferId);
				if ((unsigned int)(channel->resID - 1067) > 4)
					volume = this->soundFxVolume;
				else
					volume = this->musicVolume;
				this->openAL_SetVolume(channel->sourceId, volume);
				OpenAL_ERROR(258);
			} else {
				channel = &this->channel[existingSlot];
				if ((unsigned int)(channel->resID - 1067) > 4)
					volume = this->soundFxVolume;
				else
					volume = this->musicVolume;
				this->openAL_SetVolume(channel->sourceId, volume);
				OpenAL_ERROR(214);
			}

			this->openAL_PlaySound(channel->sourceId, (flags & 1) ? 1 : 0);
		}
	}
}

int Sound::getFreeSlot(int minPriority) {
	int slotIdx;
	int bestSlot;
	int lowestPriority;
	int chanPriority;
	bool isBetter;

	slotIdx = 0;
	bestSlot = -1;
	lowestPriority = 6;
	while (this->channel[slotIdx].resID != -1) {
		ALint state;
		alGetSourcei(this->channel[slotIdx].sourceId, AL_SOURCE_STATE, &state);
		if (state != AL_PLAYING && state != AL_PAUSED) {
			break;
		}
		chanPriority = this->channel[slotIdx].priority;
		isBetter = chanPriority < minPriority;
		if (chanPriority < minPriority)
			isBetter = chanPriority < lowestPriority;
		if (isBetter)
			bestSlot = slotIdx;
		++slotIdx;
		if (isBetter)
			lowestPriority = chanPriority;
		if (slotIdx == 10)
			return bestSlot;
	}
	return slotIdx;
}

void Sound::soundStop() {
	if (this->headless) { return; }
	for (int i = 0; i < 10; i++) {
		alSourceStop(this->channel[i].sourceId);
		this->channel[i].priority = 1;
		this->channel[i].fadeInProgress = false;
	}
}

void Sound::stopSound(int resID, bool fadeOut) {
	if (this->headless) { return; }
	int volume;
	for (int i = 0; i < 10; i++) {
		if (this->channel[i].resID == resID) {
			if (fadeOut) {
				if (!this->channel[i].fadeInProgress) {
					if ((unsigned int)((int16_t)resID - 1067) > 4)
						volume = this->soundFxVolume;
					else
						volume = this->musicVolume;
					this->channel[i].StartFade(volume, 0, 500);
				}
			} else {
				alSourceStop(this->channel[i].sourceId);
				this->channel[i].priority = 1;
				this->channel[i].fadeInProgress = false;
			}
			return;
		}
	}
}

bool Sound::isSoundPlaying(int16_t resID) {
	if (this->headless) { return false; }
	for (int i = 0; i < 10; i++) {
		if (this->channel[i].resID == resID) {
			return this->openAL_IsPlaying(this->channel[i].sourceId);
		}
	}
	return false;
}

void Sound::updateVolume() {
	if (this->headless) { return; }
	int volume;
	if (this->soundsLoaded) {
		for (int i = 0; i < 10; i++) {
			if (!this->channel[i].fadeInProgress) {
				if ((unsigned int)(this->channel[i].resID - 1067) > 4) {
					volume = this->soundFxVolume;
				} else {
					volume = this->musicVolume;
				}
				this->openAL_SetVolume(this->channel[i].sourceId, volume);
			}
		}

		this->allowMusics = (this->musicVolume != 0) ? true : false;
		this->allowSounds = (this->soundFxVolume != 0) ? true : false;
	}
}

void Sound::playCombatSound(int16_t resID, uint8_t flags, int priority) {
	this->playSound(resID, flags, priority, false);
}

bool Sound::cacheCombatSound(int resID) {
	// no implementado
	return true;
}
void Sound::freeMonsterSounds() {
	// no implementado
}

void Sound::volumeUp(int volume) {
	this->soundFxVolume += volume;
	if (this->soundFxVolume > 100) {
		this->soundFxVolume = 100;
	}
	this->updateVolume();
}

void Sound::volumeDown(int volume) {
	this->soundFxVolume -= volume;
	if (this->soundFxVolume < 0) {
		this->soundFxVolume = 0;
	}
	this->updateVolume();
}

void Sound::startFrame() {
	if (this->hasDeferredSound != 0) {
		if (!this->isMuted) {
			this->playSound(this->resID, this->flags, this->priority, this->isMuted);
			this->unused_0x160 = -1;
			this->resID = -1;
			this->flags = 0;
			this->priority = 0;
			this->unused_0x15c = -1;
			this->hasDeferredSound = 0;
		}
	}
}

void Sound::endFrame() {
	// no implementado
}

void Sound::SoundStream::StartFade(int volume, int fadeBeg, int fadeEnd) {
	this->fadeBeg = fadeBeg;
	this->fadeInProgress = true;
	this->volume = volume;
	this->fadetime = CAppContainer::getInstance()->app->gameTime;
	this->fadeVolume = (float)(fadeBeg - volume) / (float)fadeEnd;
}

void Sound::musicVolumeUp(int volume) { // [GEC]
	this->musicVolume += volume;
	if (this->musicVolume > 100) {
		this->musicVolume = 100;
	}
	this->updateVolume();
}

void Sound::musicVolumeDown(int volume) { // [GEC]
	this->musicVolume -= volume;
	if (this->musicVolume < 0) {
		this->musicVolume = 0;
	}
	this->updateVolume();
}
