#include <iostream>
#include <string>
#include <cstring>
#include <cstdint>
#include <random>

#include "ronnlib.h"

Model::Model(const Params& params) {

        inputs = params.nInputs;
        outputs = params.nOutputs;
        layers = params.nLayers;
        channels = params.nChannels;
        kernelWidth = params.kWidth;
        bias = params.useBias;
        activation = static_cast<Activation>(int(params.act));
        initType = static_cast<InitType>(int(params.init));
        dilationFactor = params.dilationFactor;
        depthwise = params.dwise;

        buildModel(params.seed);
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
        }
    }

    initModel(seed);
}

void Model::initModel(int seed){
    std::default_random_engine g1 ((unsigned) seed);
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

    for (size_t i = 0; i < model->layers.size(); i++) {
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
                for (size_t j = 0; j < w.size(); ++j)
                    w[j] = rng();
            }
        }

        layer->setWeights(convWeights);

        // load biases
        std::vector<float> convBias (layer->out_size, 0.0f);
        if (bias)
        {
            for (size_t j = 0; j < convBias.size(); ++j)
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
