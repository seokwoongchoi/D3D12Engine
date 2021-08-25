#pragma once

class Time
{
public:
	static Time* Get();

	static void Create();
	static void Delete();

	static bool Stopped() { return isTimerStopped; }
	static float Delta() { return isTimerStopped ? 0.0f : timeElapsed* GameSpeed; }
	static void SetGameSpeed(float speed) { GameSpeed = speed; }
	void Update();
	void Print();

	void Start();
	void Stop();

	inline uint GetFrameCount()const {return frameCount;}
	float FPS() const { return framePerSecond; }
	inline float Running() const { return runningTime; }

private:
	Time(void);
	~Time(void);

	static Time* instance;///< �̱��� ��ü

	static bool isTimerStopped;///< Ÿ�̸� ����
	static float timeElapsed;///< ���� ���������κ��� ����ð�
	static float GameSpeed;

	INT64 ticksPerSecond;///< �ʴ� ƽī��Ʈ
	INT64 currentTime;///< ���� �ð�
	INT64 lastTime;///< �����ð�
	INT64 lastFPSUpdate;///< ������ FPS ������Ʈ �ð�
	INT64 fpsUpdateInterval;///< fps ������Ʈ ����


	

	UINT frameCount;///< ������ ��
	float runningTime;///< ���� �ð�
	float framePerSecond;///< FPS
};