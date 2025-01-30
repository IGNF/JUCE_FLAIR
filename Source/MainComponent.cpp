#include "MainComponent.h"
#include "AppUtil.h"
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

  addAndMakeVisible(m_InputImage);
  addAndMakeVisible(m_OutputImage);

  m_sldRow.setSliderStyle(juce::Slider::LinearHorizontal);
  m_sldRow.setRange(175000., 194000., 1.);
 m_sldRow.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 100, 30);
  m_sldRow.setTextValueSuffix(juce::translate(" : Row"));
  m_sldRow.addListener(this);
  addAndMakeVisible(m_sldRow);

  m_sldCol.setSliderStyle(juce::Slider::LinearHorizontal);
  m_sldCol.setRange(255000., 273000., 1.);
  m_sldCol.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 100, 30);
  m_sldCol.setTextValueSuffix(juce::translate(" : Col"));
  m_sldCol.addListener(this);
  addAndMakeVisible(m_sldCol);

  m_sldRow.setValue(m_nTileRow, juce::NotificationType::dontSendNotification);
  m_sldCol.setValue(m_nTileCol, juce::NotificationType::dontSendNotification);

  setSize (800, 600);
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
  m_InputImage.setBounds(5, 5, b.getWidth() / 2 - 10, b.getHeight() - 100);
  m_OutputImage.setBounds(b.getWidth() / 2 + 5, 5, b.getWidth() / 2 - 10, b.getHeight() - 100);
  m_btnLoadModel.setBounds(5, b.getHeight() - 30, 80, 30);
  m_lblModel.setBounds(85, b.getHeight() - 30, 200, 30);
  m_sldRow.setBounds(300, b.getHeight() - 30, 200, 30);
  m_sldCol.setBounds(550, b.getHeight() - 30, 200, 30);
}

//-----------------------------------------------------------------------------
// Reponse aux boutons
//-----------------------------------------------------------------------------
void 	MainComponent::buttonClicked(juce::Button* button)
{
  if (button == &m_btnLoadModel)
    LoadModel();
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

  int total_number_elements = input_shape[3] * input_shape[2] * input_shape[1];
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
      input_tensor_values[cmpt + 2 * channel_size] = (line[0] - 101.82) / 44.;
      input_tensor_values[cmpt + channel_size] = (line[1] - 110.87) / 45.38;
      input_tensor_values[cmpt] = (line[2] - 105.08) / 52.17;
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
    juce::Image outImage(juce::Image::ARGB, output_shape[2], output_shape[3], true);
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
        switch (index) {
        case 0: // building
          *rgba = 0xdb0e9a; break;
        case 1: // pervious surface
          *rgba = 0x938e7b; break;
        case 2: // impervious surface
          *rgba = 0xf80c00; break;
        case 3: // bare soil
          *rgba = 0xa97101; break;
        case 4: // water
          *rgba = 0x1553ae; break;
        case 5: // coniferous
          *rgba = 0x194a26; break;
        case 6: // deciduous
          *rgba = 0x46e483; break;
        case 7: // brushwood
          *rgba = 0xf3a60d; break;
        case 8: // vineyard
          *rgba = 0x660082; break;
        case 9: // herbaceous vegetation
          *rgba = 0x55ff00; break;
        case 10: // agricultural land
          *rgba = 0xfff30d; break;
        case 11: // plowed land
          *rgba = 0xe4df7c; break;
        case 12: // swimming_pool
          *rgba = 0x3de6eb; break;
        case 13: // snow
          *rgba = 0xffffff; break;
        case 14: // clear cut
          *rgba = 0x8ab3a0; break;
        case 15: // mixed
          *rgba = 0x6b714f; break;
        case 16: // ligneous
          *rgba = 0xc5dc42; break;
        case 17: // greenhouse
          *rgba = 0x9999ff; break;
        default:
          *rgba = 0;
        }
         
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