#include <daisy_seed.h>

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
    daisy::DaisySeed hardware;
    hardware.Init();
    hardware.StartAudio(processAudioBlock);
    while(true) {}
}
