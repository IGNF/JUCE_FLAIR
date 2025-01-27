#pragma once

#include <JuceHeader.h>
#include <onnxruntime_cxx_api.h>


//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::Component, public juce::Button::Listener, public juce::Slider::Listener
{
public:
  //==============================================================================
  MainComponent();
  ~MainComponent() override;

  //==============================================================================
  void paint (juce::Graphics&) override;
  void resized() override;

  void buttonClicked(juce::Button*) override;
  bool keyPressed(const juce::KeyPress& key) override;
  void sliderValueChanged(juce::Slider* slider) override;

private:
  Ort::Session* m_Session;
  int m_nTileRow;
  int m_nTileCol;
  juce::File m_Cache; // Cache des images telechargees

  // Elements d'interface
  juce::TextButton  m_btnLoadModel;
  juce::Label m_lblModel;
  juce::ImageComponent m_InputImage;
  juce::ImageComponent m_OutputImage;
  juce::Slider m_sldRow;
  juce::Slider m_sldCol;

  bool LoadModel();
  bool LoadInputImage();
  juce::String LoadTile(int col, int row);
  bool RunModel();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
