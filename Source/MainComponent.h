#pragma once

#include <JuceHeader.h>
#include <onnxruntime_cxx_api.h>
#include "AppUtil.h"


//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::Component, public juce::Button::Listener, public juce::Slider::Listener, 
                       public juce::ComboBox::Listener, public juce::ActionListener, private juce::Timer
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
  void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
  void actionListenerCallback(const juce::String& message) override;
  void mouseDoubleClick(const juce::MouseEvent& event) override;
  void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

private:
  class LegendButton : public ColourChangeButton, public juce::ActionBroadcaster {
  public:
    void 	colourChanged() override { sendActionMessage("UpdateColors"); }
  };

  Ort::Session* m_Session;
  int m_nTileRow;
  int m_nTileCol;
  int m_nFactor;
  int m_nAnime;
  juce::File m_Cache; // Cache des images telechargees
  int m_nNbTag = 18;
  uint32_t m_Colors[18] = { 0xffdb0e9a , 0xff938e7b , 0xfff80c00 , 0xffa97101 , 0xff1553ae , 0xff194a26 , 0xff46e483 , 0xfff3a60d , 0xff660082 ,
                            0xff55ff00 , 0xfffff30d , 0xffe4df7c , 0xff3de6eb , 0xffffffff , 0xff8ab3a0 , 0xff6b714f , 0xffc5dc42 , 0xff9999ff };
  juce::String m_Tag[18] = { "building" , "pervious surface" , "impervious surface", "bare soil" , "water" , "coniferous" ,
                             "deciduous" , "brushwood" , "vineyard" , "herbaceous vegetation" , "agricultural land" , "plowed land",
                             "swimming_pool" , "snow" , "clear cut" , "mixed" , "ligneous" , "greenhouse" };
  bool m_State[18];

  // Elements d'interface
  juce::Image m_Inference;
  juce::Image m_Input;
  juce::TextButton  m_btnLoadModel;
  juce::TextButton  m_btnAbout;
  juce::Label m_lblModel;
  juce::ImageComponent m_InputImage;
  juce::ImageComponent m_OutputImage;
  juce::Slider m_sldRow;
  juce::Slider m_sldCol;
  juce::ComboBox m_cbxPlace;
  LegendButton m_btnColors[18];
  juce::ToggleButton m_btnState[18];

  void CreateLegend();
  bool LoadLegend();
  void SaveLegend();
  bool LoadModel();
  bool LoadInputImage(int factor = 1);
  juce::String LoadTile(int col, int row, int level);
  bool RunModel();

  void timerCallback() override;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
