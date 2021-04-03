#include <iostream>
#include <string>
#include <cstring>
#include <cstdint>
#include <random>

#include "ronnlib.h"

Model::Model(int nInputs, 
             int nOutputs, 
             int nLayers, 
             int nChannels, 
             int kWidth,
             int dFactor, 
             bool useBias, 
             int act,
             int init,
             int seed,
             bool dwise) {

        inputs = nInputs;
        outputs = nOutputs;
        layers = nLayers;
        channels = nChannels;
        kernelWidth = kWidth;
        bias = useBias;
        activation = static_cast<Activation>(int(act));
        initType = static_cast<InitType>(int(init));
        dilationFactor = dFactor;
        depthwise = dwise;

        buildModel(seed);
}

void Model::buildModel(int seed) {

    int inChannels, outChannels;

    model = std::make_unique<RTNeural::Model<float>> (getInputs());

    // construct the convolutional layers
    for (int i = 0; i < getLayers(); i++) 
    {
        if (i == 0 && getLayers() > 1) 
        {
            inChannels = getInputs();
            outChannels = getChannels();
        }
        else if (i == 0) 
        {
            inChannels = getInputs();
            outChannels = getOutputs(); 
        }
        else if (i + 1 == getLayers()) 
        {
            inChannels = getChannels();
            outChannels = getOutputs();
        }
        else 
        {
            inChannels = getChannels();
            outChannels = getChannels();
        }
        if (true) // (!depthwise) 
        {
            auto conv1D = std::make_unique<RTNeural::Conv1D<float>> ((size_t) inChannels,
                                                                     (size_t) outChannels,
                                                                     (size_t) getKernelWidth(),
                                                                     (size_t) std::pow (getDilationFactor(), i));
            model->addLayer (conv1D.release());

            if (i + 1 < getLayers()) {
                std::unique_ptr<RTNeural::Activation<float>> act;
                switch (getActivation()) {
                    case Linear:
                        break;
                    case LeakyReLU: // @TODO
                        break;
                    case Tanh:
                        act = std::make_unique<RTNeural::TanhActivation<float>> ((size_t) outChannels);
                        model->addLayer (act.release());
                        break;
                    case Sigmoid:
                        act = std::make_unique<RTNeural::SigmoidActivation<float>> ((size_t) outChannels);
                        model->addLayer (act.release());
                        break;
                    case ReLU:
                        act = std::make_unique<RTNeural::ReLuActivation<float>> ((size_t) outChannels);
                        model->addLayer (act.release());
                        break;
                    case ELU: // @TODO
                        break;
                    case SELU: // @TODO
                        break;
                    case GELU: // @TODO
                        break;
                    case RReLU: // @TODO
                        break;
                    case Softplus: // @TODO
                        break;
                    case Softshrink: // @TODO
                        break;
                    case Sine: // @TODO
                        break;
                    case Sine30: // @TODO
                        break;
                    default:
                        break;
                }
            }
        }
        else 
        {   // first depthwise conv
            int groups;
            if (i+1 == getLayers())
            {
                groups = 1;
            }
            else if (i == 0) 
            {
                groups = 1;
            }
            else 
            {
                groups = inChannels;
            }
            // @TODO: not sure how to implement yet with RTNeural...
            std::cout << i << " " << inChannels << " " << outChannels << " " << groups << std::endl;
            // conv.push_back(torch::nn::Conv1d(
            //     torch::nn::Conv1dOptions(inChannels,outChannels,getKernelWidth())
            //     .stride(1)
            //     .groups(groups)
            //     .dilation(pow(getDilationFactor(),i))
            //     .bias(getBias())));

            // followed by pointwise conv
            //conv.push_back(torch::nn::Conv1d(
            //    torch::nn::Conv1dOptions(outChannels,outChannels,1)
            //    .stride(1)
            //    .bias(getBias())));
        }
    }

    initModel(seed);
}

// the forward operation
// torch::Tensor Model::forward(torch::Tensor x) {
//     // we iterate over the convolutions
//     for (auto i = 0; i < getLayers(); i++) {
//         if (i + 1 < getLayers()) {
//             //setActivation(static_cast<Activation>(rand() % Sine));
//             switch (getActivation()) {
//                 case Linear:        x =                   (conv[i](x)); break;
//                 case LeakyReLU:     x = leakyrelu         (conv[i](x)); break;
//                 case Tanh:          x = torch::tanh       (conv[i](x)); break;
//                 case Sigmoid:       x = torch::sigmoid    (conv[i](x)); break;
//                 case ReLU:          x = torch::relu       (conv[i](x)); break;
//                 case ELU:           x = torch::elu        (conv[i](x)); break;
//                 case SELU:          x = torch::selu       (conv[i](x)); break;
//                 case GELU:          x = torch::gelu       (conv[i](x)); break;
//                 case RReLU:         x = torch::rrelu      (conv[i](x)); break;
//                 case Softplus:      x = torch::softplus   (conv[i](x)); break;
//                 case Softshrink:    x = torch::softshrink (conv[i](x)); break;
//                 case Sine:          x = torch::sin        (conv[i](x)); break;
//                 case Sine30:        x = torch::sin        (30 * conv[i](x)); break;
//                 default:            x =                   (conv[i](x)); break;
//             }
//         }
//         else
//             x = conv[i](x);
//     }
//     return x;
// }

void Model::initModel(int seed){
    std::mt19937 g1 ((unsigned) seed);
    std::function<float()> rng;
    
    std::normal_distribution<float> normalDist {};
    std::uniform_real_distribution<float> un1Dist { -0.25f, 0.25f };
    std::uniform_real_distribution<float> un2Dist { -1.0f, 1.0f };

    switch(getInitType())
    {
        case normal:
            rng = std::bind (normalDist, g1);
            break;
        case uniform1:
            rng = std::bind (un1Dist, g1);
            break;
        case uniform2:
            rng = std::bind (un2Dist, g1);
            break;
        case xavier_normal: // @TODO
            return;
        case xavier_uniform: // @TODO
            return;
        case kaiming_normal: // @TODO
            return;
        case kamming_uniform: // @TODO
            return;
    }

    for (auto i = 0; i < model->layers.size(); i++) {
        auto* layer = dynamic_cast<RTNeural::Conv1D<float>*> (model->layers[i]);
        if (layer == nullptr) // not a Conv1D layer!
            continue;

        std::vector<std::vector<std::vector<float>>> convWeights(layer->out_size);
        for(auto& wIn : convWeights)
        {
            wIn.resize(layer->in_size);

            for(auto& w : wIn)
            {
                w.resize(layer->getKernelSize(), 0.0f);
                for (auto j = 0; j < w.size(); ++j)
                    w[j] = rng();
            }
        }

        layer->setWeights(convWeights);

        // load biases
        std::vector<float> convBias (layer->out_size, 0.0f);
        if (bias)
        {
            for (auto j = 0; j < convBias.size(); ++j)
                convBias[j] = rng();
        }

        layer->setBias(convBias);
    }
}

int Model::getOutputSize(int frameSize){
    int outputSize = frameSize;
    for (auto i = 0; i < getLayers(); i++) {
        outputSize = outputSize - ((getKernelWidth()-1) * pow(getDilationFactor(), i));
    }
    return outputSize;
}

int Model::getNumParameters(){
    return 100;

    // @TODO
    // int n = 0;
    // for (const auto& p : parameters()) {
    //     auto sizes = p.sizes();
    //     int s = 1;
    //     for (auto dim : sizes) {
    //         std::cout << dim << std::endl;
    //         s = s * dim;
    //     }
    //     n = n + s;
    // }
    // return n;
}