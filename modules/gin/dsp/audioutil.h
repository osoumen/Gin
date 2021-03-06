/*
  ==============================================================================

    AudioUtil.h
    Created: 24 Jan 2018 7:06:35pm
    Author:  Roland Rabien

  ==============================================================================
*/

#pragma once

//==============================================================================
inline Value findValue (ValueTree& state, Identifier name, var value)
{
    if (state.hasProperty (name))
        return state.getPropertyAsValue (name, nullptr);
    
    state.setProperty(name, value, nullptr);
    return state.getPropertyAsValue (name, nullptr);
}

//==============================================================================
class AudioFifo
{
public:
    AudioFifo (int channels, int numSamples)
      : fifo (numSamples), buffer (channels, numSamples)
    {
    }
    
    void setSize (int numChannels, int numSamples)
    {
        fifo.setTotalSize (numSamples);
        buffer.setSize (numChannels, numSamples);
    }
    
    int getFreeSpace() const noexcept       { return fifo.getFreeSpace(); }
    int getNumReady() const noexcept        { return fifo.getNumReady(); }
    void reset() noexcept                   { fifo.reset(); }
    
    void ensureFreeSpace (int numSamples)
    {
        const int freeSpace = getFreeSpace();
        if (numSamples > freeSpace)
        {
            const int samplesRequired = numSamples - freeSpace;
            jassert (samplesRequired <= getNumReady());
            fifo.finishedRead (samplesRequired);
        }
    }
    
    bool write (const AudioSampleBuffer& src)
    {
        return write (src.getArrayOfReadPointers(), src.getNumSamples());
    }
    
    bool write (const float* const* data, int numSamples)
    {
        if (numSamples <= 0)
            return true;
        
        int start1, size1, start2, size2;
        fifo.prepareToWrite (numSamples, start1, size1, start2, size2);
        
        if (size1 + size2 < numSamples)
            return false;
        
        for (int i = buffer.getNumChannels(); --i >= 0;)
        {
            buffer.copyFrom (i, start1, data[i], size1);
            buffer.copyFrom (i, start2, data[i] + size1, size2);
        }
        
        fifo.finishedWrite (size1 + size2);
        return true;
    }
    
    bool read (AudioSampleBuffer& dest, int startSampleInDestBuffer)
    {
        return read (dest, startSampleInDestBuffer, dest.getNumSamples());
    }
    
    bool read (AudioSampleBuffer& dest, int startSampleInDestBuffer, int numSamples)
    {
        int start1, size1, start2, size2;
        fifo.prepareToRead (numSamples, start1, size1, start2, size2);
        
        if ((size1 + size2) < numSamples)
            return false;
        
        for (int i = buffer.getNumChannels(); --i >= 0;)
        {
            dest.copyFrom (i, startSampleInDestBuffer, buffer, i, start1, size1);
            dest.copyFrom (i, startSampleInDestBuffer + size1, buffer, i, start2, size2);
        }
        
        fifo.finishedRead (size1 + size2);
        return true;
    }
    
    bool readAdding (AudioSampleBuffer& dest, int startSampleInDestBuffer)
    {
        return readAdding (dest, startSampleInDestBuffer, dest.getNumSamples());
    }
    
    bool readAdding (AudioSampleBuffer& dest, int startSampleInDestBuffer, int numSamples)
    {
        int start1, size1, start2, size2;
        fifo.prepareToRead (numSamples, start1, size1, start2, size2);
        
        if ((size1 + size2) < numSamples)
            return false;
        
        for (int i = buffer.getNumChannels(); --i >= 0;)
        {
            dest.addFrom (i, startSampleInDestBuffer, buffer, i, start1, size1);
            dest.addFrom (i, startSampleInDestBuffer + size1, buffer, i, start2, size2);
        }
        
        fifo.finishedRead (size1 + size2);
        return true;
    }
    
private:
    AbstractFifo fifo;
    AudioSampleBuffer buffer;
    
    JUCE_DECLARE_NON_COPYABLE (AudioFifo)
};
