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

struct VariantData
{
	char * filename;
};

struct SoundData
{
	std::vector<VariantData> variants;
};

class Sound
{
public:
    
    Sound()
    {
        // ...
    }
    
    void LoadSound( Sounds soundIndex, char filename[] )
    {
		SoundData & sound = sounds[soundIndex];
		VariantData variant;
		variant.filename = filename;
		sound.variants.push_back( variant );
    }
    
	void PlaySound( Sounds soundIndex )
	{
		const char * soundName = soundNames[soundIndex];
		printf( "play sound: %s\n", soundName );
		SoundData & sound = sounds[soundIndex];
		int numVariants = sound.variants.size();
		if ( numVariants > 0 )
		{
			int i = rand() % numVariants;
			printf( " --> %s\n", sound.variants[i].filename );
		}
		else
			printf( " --> no variants defined\n" );
	}

private:

	SoundData sounds[SOUND_NumValues];
};

#endif
