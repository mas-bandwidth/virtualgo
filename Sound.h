#ifndef SOUND_H
#define SOUND_H

enum Sounds
{
	SOUND_PlaceStone,
	SOUND_PickUpStone,
	SOUND_FlickStone,
	SOUND_Swipe,
	SOUND_NumValues
};

const char * soundNames[] =
{
	"place stone",
	"pick up stone",
	"flick stone",
	"swipe"
};

class Sound
{
public:
    
    Sound()
    {
        // ...
    }
    
    void AddSound( Sounds sound, char filename[] )
    {
        // ...
    }
    
	void PlaySound( Sounds sound )
	{
		const char * soundName = soundNames[sound];
		printf( "play sound: %s\n", soundName );
	}
};

#endif
