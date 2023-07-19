#include "stdafx.h"
#include "Audio.h"
#include "Player.h"
#include "LibData.h"
#include "Exception.h"
#include "mpg123.h"

namespace sound {

	AudioMgr::AudioMgr() : wait(null), soundDevice() {
		players = new (this) WeakSet<Player>();

		try {
			init();
		} catch (const storm::Exception *e) {
			std::wcout << e->toS() << endl;
			soundDevice = SoundDevice();
		} catch (const ::Exception &e) {
			std::wcout << e.what() << endl;
			soundDevice = SoundDevice();
		}

		// We need to initialize the MP3 backend.
		mpg123_init();
	}

	void AudioMgr::terminate() {
		if (wait) {
			wait->terminate();
			wait = null;
		}

		// We're likely running in a different thread that the thread that set up OpenAL. We need to
		// activate the OpenAL context once more to make 'close' work as expected.
		activate();

		WeakSet<Player>::Iter i = players->iter();
		while (Player *p = i.next()) {
			p->close();
		}

		destroy();
	}

	void AudioMgr::addPlayer(Player *player) {
		players->put(player);
	}

	void AudioMgr::removePlayer(Player *player) {
		players->remove(player);
	}

	AudioMgr *audioMgr(EnginePtr e) {
		AudioMgr *&d = audioMgrData(e.v);
		if (!d)
			d = new (e.v) AudioMgr();
		return d;
	}

	/**
	 * Waiting logic.
	 */

	AudioWait::AudioWait(Engine &e) : e(e), exit(false), working(false), notifyExit(null) {
		AudioMgr *m = audioMgr(e);
		m->wait = this;
	}

	AudioWait::~AudioWait() {
		destroy();
	}

	void AudioWait::doExit() {
		if (exit && !working && notifyExit) {
			notifyExit->up();
			notifyExit = null;
		}
	}

	void AudioWait::terminate() {
		os::Sema sync(0);
		notifyExit = &sync;

		exit = true;

		// Allow the thread to exit properly...
		signal();
		sync.down();
	}

	os::Thread spawnAudioThread(Engine &e) {
		return os::Thread::spawn(new AudioWait(e), runtime::threadGroup(e));
	}

	/**
	 * Backend specific code.
	 */
#ifdef SOUND_DX

	void AudioMgr::init() {
		IDirectSound8 *dsound = null;
		if (FAILED(DirectSoundCreate8(NULL, &dsound, NULL))) {
			throw new (this) SoundInitError();
		} else {
			// This is fine since we do not do anything that depends on focus:
			// https://groups.google.com/forum/#!msg/microsoft.public.win32.programmer.directx.audio/a7QhlgNFBpg/w8lTd8_NRSEJ
			dsound->SetCooperativeLevel(GetDesktopWindow(), DSSCL_PRIORITY);
			soundDevice = dsound;
		}
	}

	void AudioMgr::destroy() {
		soundDevice.release();
	}

	void AudioMgr::activate() {}

	static bool signaled(HANDLE event) {
		return WaitForSingleObjectEx(event, 0, FALSE) != WAIT_TIMEOUT;
	}

	void AudioMgr::notifyEvents() {
		WeakSet<Player>::Iter i = players->iter();
		while (Player *p = i.next()) {
			if (p->waitEvent() == Handle())
				continue;

			HANDLE event = p->waitEvent().v();
			if (signaled(event)) {
				ResetEvent(event);
				p->onNotify();
			}
		}
	}

	void AudioMgr::allEvents(vector<HANDLE> &all) {
		all.resize(2);
		WeakSet<Player>::Iter i = players->iter();
		while (Player *p = i.next()) {
			Handle event = p->waitEvent();
			if (event != Handle())
				all.push_back(event.v());
		}
	}

	void AudioWait::init() {
		events.push_back(NULL);
		events.push_back(CreateEvent(NULL, TRUE, FALSE, NULL));
	}

	void AudioWait::destroy() {
		CloseHandle(events[1]);
	}

	bool AudioWait::wait(os::IOHandle &io) {
		return wait(io, INFINITE);
	}

	bool AudioWait::wait(os::IOHandle &io, nat ms) {
		if (exit) {
			doExit();
			return false;
		}

		events[0] = io.v();
		Nat first = events[0] ? 0 : 1;
		Nat count = 2;
		if (!working) {
			AudioMgr *m = audioMgr(e);
			m->allEvents(events);
			count = nat(events.size());
		}

		DWORD z = WaitForMultipleObjectsEx(count - first, &events[first], FALSE, ms, FALSE);
		ResetEvent(events[1]);

		if (exit)
			doExit();
		return !exit;
	}

	void AudioWait::signal() {
		SetEvent(events[1]);
	}

	void AudioWait::work() {
		AudioMgr *m = audioMgr(e);
		working = true;
		m->notifyEvents();
		working = false;
		doExit();
	}

#endif
#ifdef SOUND_AL

	/**
	 * Note: There does not seem to be any sophisticated mechanism to get notified when sound
	 * buffers in OpenAL are finished playing. So we simply make sure that we do not sleep longer
	 * than a couple of hundred ms each time, so that we can poll OpenAL often enough.
	 */
	static const nat MAX_WAIT_MS = 100;

	void AudioMgr::init() {
		soundDevice = alcOpenDevice(NULL);
		if (!soundDevice)
			throw new (this) SoundInitError();

		soundContext = alcCreateContext(soundDevice, NULL);
		if (!soundContext)
			throw new (this) SoundInitError();

		alcMakeContextCurrent(soundContext);

		// Set up the listener.
		alListener3f(AL_POSITION, 0, 0, 1);
		alListener3f(AL_VELOCITY, 0, 0, 0);
		ALfloat orientation[] = { 0, 0, 1, 0, 1, 0 };
		alListenerfv(AL_ORIENTATION, orientation);
	}

	void AudioMgr::destroy() {
		alcDestroyContext(soundContext);
		alcCloseDevice(soundDevice);
	}

	void AudioMgr::activate() {
		if (soundContext)
			alcMakeContextCurrent(soundContext);
	}

	void AudioMgr::notifyEvents() {
		WeakSet<Player>::Iter i = players->iter();
		while (Player *p = i.next()) {
			if (p->playing())
				p->onNotify();
		}
	}

	bool AudioMgr::anyPlayback() {
		WeakSet<Player>::Iter i = players->iter();
		while (Player *p = i.next()) {
			if (p->playing())
				return true;
		}

		return false;
	}

	void AudioWait::init() {}

	void AudioWait::destroy() {}

	bool AudioWait::wait(os::IOHandle &io) {
		if (exit) {
			doExit();
			return false;
		}

		if (!working && audioMgr(e)->anyPlayback()) {
			ioWait.wait(io, MAX_WAIT_MS);
		} else {
			ioWait.wait(io);
		}

		if (exit)
			doExit();
		return !exit;
	}

	bool AudioWait::wait(os::IOHandle &io, nat ms) {
		if (exit) {
			doExit();
			return false;
		}

		if (!working && audioMgr(e)->anyPlayback()) {
			ioWait.wait(io, min(MAX_WAIT_MS, ms));
		} else {
			ioWait.wait(io, ms);
		}

		if (exit)
			doExit();
		return !exit;
	}

	void AudioWait::signal() {
		ioWait.signal();
	}

	void AudioWait::work() {
		AudioMgr *m = audioMgr(e);
		working = true;
		m->notifyEvents();
		working = false;
		doExit();
	}

#endif

}
