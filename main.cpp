#include <daisy_seed.h>

#include <util/Terrarium.h>

//=============================================================================
void processAudioBlock(
    daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out,
    size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        out[0][i] = in[0][i];
        out[1][i] = in[1][i];
    }
}

//=============================================================================
int main()
{
    Terrarium terrarium;
    terrarium.Init();
    terrarium.seed.StartAudio(processAudioBlock);
    terrarium.Loop(30, [&](){
        terrarium.leds[0].Set(terrarium.knobs[3].Process());
        terrarium.leds[1].Set(terrarium.knobs[5].Process());
    });
}
