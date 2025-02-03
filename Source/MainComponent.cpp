#include "MainComponent.h"
#include <vector>

//-----------------------------------------------------------------------------
// Fonction utilitaire
//-----------------------------------------------------------------------------
template <typename T>
Ort::Value vec_to_tensor(std::vector<T>& data, const std::vector<std::int64_t>& shape) {
  Ort::MemoryInfo mem_info =
    Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);
  auto tensor = Ort::Value::CreateTensor<T>(mem_info, data.data(), data.size(), shape.data(), shape.size());
  return tensor;
}

//-----------------------------------------------------------------------------
// Constructeur
//-----------------------------------------------------------------------------
MainComponent::MainComponent()
{
  m_Session = nullptr;
  m_nTileRow = 181013;
  m_nTileCol = 266099;
  setWantsKeyboardFocus(true);

  // Creation du repertoire cache
  juce::File tmpDir = juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory);
  m_Cache = tmpDir.getNonexistentChildFile(juce::File::createLegalFileName("JUCE_FLAIR"), "");
  m_Cache.createDirectory();

  m_btnLoadModel.setButtonText(juce::translate("Load Model"));
  m_btnLoadModel.addListener(this);
  addAndMakeVisible(m_btnLoadModel);
  addAndMakeVisible(m_lblModel);

  m_btnAbout.setButtonText(juce::translate("About"));
  m_btnAbout.addListener(this);
  addAndMakeVisible(m_btnAbout);

  addAndMakeVisible(m_InputImage);
  addAndMakeVisible(m_OutputImage);

  m_sldRow.setSliderStyle(juce::Slider::LinearHorizontal);
  m_sldRow.setRange(175000., 194000., 1.);
  m_sldRow.setTextBoxStyle(juce::Slider::TextBoxAbove, true, 100, 25);
  m_sldRow.setTextValueSuffix(juce::translate(" : Row"));
  m_sldRow.addListener(this);
  addAndMakeVisible(m_sldRow);

  m_sldCol.setSliderStyle(juce::Slider::LinearHorizontal);
  m_sldCol.setRange(255000., 273000., 1.);
  m_sldCol.setTextBoxStyle(juce::Slider::TextBoxAbove, true, 100, 25);
  m_sldCol.setTextValueSuffix(juce::translate(" : Col"));
  m_sldCol.addListener(this);
  addAndMakeVisible(m_sldCol);

  m_sldRow.setValue(m_nTileRow, juce::NotificationType::dontSendNotification);
  m_sldCol.setValue(m_nTileCol, juce::NotificationType::dontSendNotification);

  m_cbxPlace.addItem("Vaux-le-Vicomte", 1);
  m_cbxPlace.addItem("IGN", 2);
  m_cbxPlace.addItem("Bernay-Vilbert", 3);
  m_cbxPlace.addItem("Berck", 4);
  m_cbxPlace.addItem("Vannes", 5);
  m_cbxPlace.addItem("Saumur", 6);
  m_cbxPlace.addListener(this);
  m_cbxPlace.setWantsKeyboardFocus(false);
  m_cbxPlace.setSelectedId(1, juce::NotificationType::dontSendNotification);
  addAndMakeVisible(m_cbxPlace);

  LoadLegend();
  for (int i = 0; i < m_nNbTag; i++) {
    m_btnColors[i].setButtonText(m_Tag[i]);
    m_btnColors[i].setColour(juce::TextButton::buttonColourId, juce::Colour(m_Colors[i]));
    addAndMakeVisible(m_btnColors[i]);
    m_btnColors[i].addActionListener(this);
  }

  setSize (1000, 600);
}

//-----------------------------------------------------------------------------
// Destructeur
//-----------------------------------------------------------------------------
MainComponent::~MainComponent()
{
  if (m_Session != nullptr)
    delete m_Session;
  m_Cache.deleteRecursively();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
  g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

//-----------------------------------------------------------------------------
// Redimmensionnement de la fenetre
//-----------------------------------------------------------------------------
void MainComponent::resized()
{
  auto b = getLocalBounds();

  m_InputImage.setBounds(5, 5, b.getWidth() / 2 - 60, b.getHeight() - 100);
  m_OutputImage.setBounds(b.getWidth() / 2 - 60 + 5, 5, b.getWidth() / 2 - 60, b.getHeight() - 100);
  for (int i = 0; i < m_nNbTag; i++)
    m_btnColors[i].setBounds(b.getWidth() - 105, 5 + 30 * i, 100, 30);

  m_btnLoadModel.setBounds(5, b.getHeight() - 35, 80, 30);
  m_lblModel.setBounds(85, b.getHeight() - 35, 200, 30);
  m_sldRow.setBounds(300, b.getHeight() - 50, 150, 45);
  m_sldCol.setBounds(460, b.getHeight() - 50, 150, 45);
  m_cbxPlace.setBounds(640, b.getHeight() - 50, 150, 45);
  m_btnAbout.setBounds(800, b.getHeight() - 35, 80, 30);
}

//-----------------------------------------------------------------------------
// Reponse aux boutons
//-----------------------------------------------------------------------------
void 	MainComponent::buttonClicked(juce::Button* button)
{
  if (button == &m_btnLoadModel)
    LoadModel();
  if (button == &m_btnAbout) {
    juce::String message = juce::String("Permet d'inférer un modele type FLAIR au format ONNX\n");
    message += juce::String("Charge des dalles orthophotos de la GéoPlateforme (à 15 cm de résolution)\n\n");
    message += juce::String("Les flèches du clavier permettent de se déplacer\n\n");
    message += juce::String("(C) 2025 IGN / DSI / SIMV\n");
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, juce::translate("About JUCE_FLAIR"), message, "OK");
  }
}

//==============================================================================
// Modification des sliders
//==============================================================================
void MainComponent::sliderValueChanged(juce::Slider* slider)
{
  if ((slider == &m_sldCol) || (slider == &m_sldRow)) {
    m_nTileCol = (int)m_sldCol.getValue();
    m_nTileRow = (int)m_sldRow.getValue();
    LoadInputImage();
  }
}

//==============================================================================
// Modification des combo-box
//==============================================================================
void MainComponent::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
  if (comboBoxThatHasChanged != &m_cbxPlace)
    return;
  switch (m_cbxPlace.getSelectedId()) {
  case 1 : // Vaux-le-Vicomte
    m_nTileRow = 181013;
    m_nTileCol = 266099;
    break;
  case 2 : // IGN
    m_nTileRow = 180395;
    m_nTileCol = 265674;
    break;
  case 3: // Bernay-Vilbert
    m_nTileRow = 180780;
    m_nTileCol = 266422;
    break;
  case 4: // Berck
    m_nTileRow = 176886;
    m_nTileCol = 264414;
    break;
  case 5: // Vannes
    m_nTileRow = 183057;
    m_nTileCol = 258099;
    break;
  case 6: // Saumur
    m_nTileRow = 183841;
    m_nTileCol = 261948;
    break;
  }
  m_sldCol.setValue(m_nTileCol, juce::NotificationType::dontSendNotification);
  m_sldRow.setValue(m_nTileRow, juce::NotificationType::dontSendNotification);
  LoadInputImage();
}

//==============================================================================
// Gestion des actions
//==============================================================================
void MainComponent::actionListenerCallback(const juce::String& message)
{
  if (message == "UpdateColors") {
    for (int i = 0; i < m_nNbTag; i++) {
      juce::Colour color = m_btnColors[i].findColour(juce::TextButton::buttonColourId);
      m_Colors[i] = color.getARGB();
    }
    LoadInputImage();
    SaveLegend();
    return;
  }
}

//==============================================================================
// Gestion du clavier
//==============================================================================
bool MainComponent::keyPressed(const juce::KeyPress& key)
{
  if ((key.getKeyCode() == juce::KeyPress::leftKey)) {
    m_nTileCol--;
    m_sldCol.setValue(m_nTileCol, juce::NotificationType::dontSendNotification);
    LoadInputImage();
    return true;
  }
  if ((key.getKeyCode() == juce::KeyPress::rightKey)) {
    m_nTileCol++;
    m_sldCol.setValue(m_nTileCol, juce::NotificationType::dontSendNotification);
    LoadInputImage();
    return true;
  }
  if ((key.getKeyCode() == juce::KeyPress::upKey)) {
    m_nTileRow--;
    m_sldRow.setValue(m_nTileRow, juce::NotificationType::dontSendNotification);
    LoadInputImage();
    return true;
  }
  if ((key.getKeyCode() == juce::KeyPress::downKey)) {
    m_nTileRow++;
    m_sldRow.setValue(m_nTileRow, juce::NotificationType::dontSendNotification);
    LoadInputImage();
    return true;
  }
  return false;	// On transmet l'evenement sans le traiter
}

//-----------------------------------------------------------------------------
// Creation d'une image pour la legende
//-----------------------------------------------------------------------------
void MainComponent::CreateLegend()
{
  juce::Image legend(juce::Image::ARGB, 100, m_nNbTag * 30, true);
  {
    juce::Graphics g(legend);
    uint32_t color;
    uint8_t* ptr = (uint8_t*)&color;
    for (int i = 0; i < m_nNbTag; i++) {
      color = m_Colors[i];
      g.setColour(juce::Colour(ptr[2], ptr[1], ptr[0], (uint8_t)255));
      g.fillRect(0, i * 30, 100, 30);
      g.setColour(juce::Colours::black);
      g.drawText(m_Tag[i], 0, i * 30, 100, 30, juce::Justification::centred);
    }
  }
}

//-----------------------------------------------------------------------------
// Chargement de la legende dans les options de l'application
//-----------------------------------------------------------------------------
bool MainComponent::LoadLegend()
{
  juce::String legend = AppUtil::GetAppOption("Legend");
  if (legend.isEmpty())
    return false;
  juce::StringArray T;
  T.addTokens(legend, ":", "");
  if (T.size() < m_nNbTag)
    return false;
  for (int i = 0; i < m_nNbTag; i++)
    m_Colors[i] = (uint32_t)T[i].getHexValue32();
  return true;
}

//-----------------------------------------------------------------------------
// Sauvegarde de la legende dans les options de l'application
//-----------------------------------------------------------------------------
void MainComponent::SaveLegend()
{
  juce::String legend;
  for (int i = 0; i < m_nNbTag; i++)
    legend += juce::String::toHexString(m_Colors[i]) + ":";
  AppUtil::SaveAppOption("Legend", legend);
}

//-----------------------------------------------------------------------------
// Chargement d'un modele
//-----------------------------------------------------------------------------
bool MainComponent::LoadModel()
{
  if (m_Session != nullptr)   // On detruit la session si un modele a deja ete charge
    delete m_Session;

  juce::String filename = AppUtil::OpenFile("Model", "Modele de classification FLAIR", "*.onnx");
  if (filename.isEmpty())
    return false;
  juce::File modelFile(filename);
  m_lblModel.setText(modelFile.getFileName(), juce::NotificationType::dontSendNotification);

#if defined JUCE_WINDOWS
  std::basic_string<ORTCHAR_T> model_file = filename.toWideCharPointer();
#else
  std::basic_string<ORTCHAR_T> model_file = filename.toStdString();
#endif

  // Creation d'une session ONNXRuntime
  Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "JUCE_FLAIR");
  Ort::SessionOptions session_options;
  m_Session = new Ort::Session(env, model_file.c_str(), session_options);

  LoadInputImage();

  return true;
}

//-----------------------------------------------------------------------------
// Chargement d'une image
//-----------------------------------------------------------------------------
bool MainComponent::LoadInputImage()
{
  juce::Image finalImage(juce::Image::RGB, 512, 512, true);
  {
    juce::Graphics g(finalImage);
    juce::String filename;
    juce::Image image;

    for (int i = 0; i < 2; i++) {
      for (int j = 0; j < 2; j++) {
        filename = LoadTile(m_nTileCol + i, m_nTileRow + j);
        image = juce::ImageFileFormat::loadFrom(juce::File(filename));
        if (image.isValid())
          g.drawImageAt(image, i * 256, j * 256);
        else {
          juce::File badFile(filename); // Le fichier est peut-etre corrompu
          badFile.deleteFile(); // On le retire du cache
        }
      }
    }
  }

  m_InputImage.setImage(finalImage);
  RunModel();

  return true;
}

//-----------------------------------------------------------------------------
// Chargement d'une dalle de la GeoPlateforme
//-----------------------------------------------------------------------------
juce::String MainComponent::LoadTile(int col, int row)
{
  juce::String filename = m_Cache.getFullPathName() + juce::File::getSeparatorString() + juce::String(col) + "_" + juce::String(row);
  filename = juce::File::createLegalPathName(filename);
  juce::File cache(filename);
  if (cache.existsAsFile()) // Le fichier a deja ete telecharge
    return filename;

  // Telechargement du fichier
  juce::String server = "https://data.geopf.fr/wmts";
  juce::String id = "ORTHOIMAGERY.ORTHOPHOTOS";
  juce::String format = "image/jpeg";
  juce::String style = "normal";
  juce::String tms = "PM";
  juce::String level = juce::String(19);
  juce::String request = server + "?LAYER=" + id + "&EXCEPTIONS=text/xml&FORMAT=" + format +
    "&SERVICE=WMTS&VERSION=1.0.0&REQUEST=GetTile&STYLE=" + style + "&TILEMATRIXSET=" + tms +
    "&TILEMATRIX=" + level + "&TILEROW=" + juce::String(row) + "&TILECOL=" + juce::String(col);

  juce::URL url(request);
  juce::URL::DownloadTaskOptions options;
  std::unique_ptr< juce::URL::DownloadTask > task = url.downloadToFile(filename, options);
  if (task.get() == nullptr)
    return filename;
  int count = 0;
  while (task.get()->isFinished() == false)
  {
    juce::Thread::sleep(50);
    count++;
    if (count > 100) break;
  }
  return filename;
}

//-----------------------------------------------------------------------------
// Application du modele sur l'image
//-----------------------------------------------------------------------------
bool MainComponent::RunModel()
{
  if (m_Session == nullptr) // Pas de modele charge
    return false;

  if ((m_Session->GetInputCount() < 1) || (m_Session->GetOutputCount() < 1))
    return false;
  std::vector<std::int64_t> input_shape = m_Session->GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape();
  std::vector<std::int64_t> output_shape = m_Session->GetOutputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape();

  if (input_shape[0] == -1) input_shape[0] = 1; // On ne traite qu'une image a la fois

  // Recuperation de l'image en entree
  juce::Image image = m_InputImage.getImage();
  juce::Image::BitmapData bitmap(image, juce::Image::BitmapData::readOnly);

  if ((bitmap.height != input_shape[3])|| (bitmap.width != input_shape[2]) || (bitmap.pixelStride < input_shape[1]))
    return false;

  int total_number_elements = (int)(input_shape[3] * input_shape[2] * input_shape[1]);
  std::vector<float> input_tensor_values(total_number_elements);
  
  // Recuperation des valeurs des pixels
  int cmpt = 0, channel_size = bitmap.height * bitmap.width;
  for (int i = 0; i < bitmap.height; i++) {
    juce::uint8* line = bitmap.getLinePointer(i);
    for (int j = 0; j < bitmap.width; j++) {
      /*
        norm_means: [105.08, 110.87, 101.82, 106.38, 53.26]
        norm_stds: [52.17, 45.38, 44, 39.69, 79.3]
      */
      // Ordre BGR dans les images JUCE
      input_tensor_values[cmpt + 2 * channel_size] = (float)((line[0] - 101.82) / 44.);
      input_tensor_values[cmpt + channel_size] = (float)((line[1] - 110.87) / 45.38);
      input_tensor_values[cmpt] = (float)((line[2] - 105.08) / 52.17);
      cmpt++;
      line += bitmap.pixelStride;
    }
  }

  // Creation du tenseur d'entree
  std::vector<Ort::Value> input_tensors;
  input_tensors.emplace_back(vec_to_tensor<float>(input_tensor_values, input_shape));

  // Creation des tableaux de chaines de caracteres
  Ort::AllocatorWithDefaultOptions allocator;
  std::vector<std::string> input_names;
  input_names.emplace_back(m_Session->GetInputNameAllocated(0, allocator).get());
  std::vector<std::string> output_names;
  output_names.emplace_back(m_Session->GetOutputNameAllocated(0, allocator).get());

  std::vector<const char*> input_names_char(input_names.size(), nullptr);
  std::transform(std::begin(input_names), std::end(input_names), std::begin(input_names_char),
    [&](const std::string& str) { return str.c_str(); });

  std::vector<const char*> output_names_char(output_names.size(), nullptr);
  std::transform(std::begin(output_names), std::end(output_names), std::begin(output_names_char),
    [&](const std::string& str) { return str.c_str(); });


  try {
    auto output_tensors = m_Session->Run(Ort::RunOptions{ nullptr }, input_names_char.data(), input_tensors.data(),
      input_names_char.size(), output_names_char.data(), output_names_char.size());

    // Creation de l'image de sortie en ARGB
    juce::Image outImage(juce::Image::ARGB, (int)output_shape[2], (int)output_shape[3], true);
    juce::Image::BitmapData outData(outImage, juce::Image::BitmapData::readWrite);
    float* arr = output_tensors.front().GetTensorMutableData<float>();

    for (int i = 0; i < outData.height; i++) {
      uint32_t* rgba = (uint32_t*)outData.getLinePointer(i);
      for (int j = 0; j < outData.width; j++) {
        float max = -1000., value;
        int index = 0;

        // Recherche de la valeur maximale
        for (int k = 0; k < output_shape[1]; k++) {
          value = *(arr + k * output_shape[2] * output_shape[3]); // Classement en plans de couleur
          if (value > max) {
            max = value;
            index = k;
          }
        }

        // Application d'une table de couleurs pour rendre la sortie jolie ...
        if (index < m_nNbTag)
          *rgba = m_Colors[index];
        else
          *rgba = 0;
         
        rgba++;
        arr++;
      }
    }
    m_OutputImage.setImage(outImage);

  }
  catch (const Ort::Exception& exception) {
    juce::String message = "ERROR running model inference: ";
    message += exception.what();
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, juce::translate("JUCE FLAIR"), message, "OK");
    return false;
  }

  return true;
}