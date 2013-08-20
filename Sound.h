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
	const char * filename;
};

struct SoundData
{
	int lastVariantPlayed;
	std::vector<VariantData> variants;
};

class Sound
{
public:
    
    Sound()
    {
        // ...
    }
    
    void LoadSound( Sounds soundIndex, const char * filename )
    {
		SoundData & sound = sounds[soundIndex];
		VariantData variant;
		variant.filename = filename;
		sound.lastVariantPlayed = -1;
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
			int i = 0;
			if ( numVariants > 1 )
			{
				do
				{
					i = rand() % numVariants;
				}
				while ( i == sound.lastVariantPlayed );
			}
			sound.lastVariantPlayed = i;
			printf( " --> %s\n", sound.variants[i].filename );
		}
		else
			printf( " --> no variants defined\n" );
	}

private:

	SoundData sounds[SOUND_NumValues];
};

#endif
