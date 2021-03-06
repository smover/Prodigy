/**
 * Code adapted from https://github.com/mlpack/models/blob/master/Kaggle/DigitRecognizer/src/DigitRecognizer.cpp
 */

#include <iostream>

#include <mlpack/core.hpp>
#include <mlpack/core/data/split_data.hpp>

#include <mlpack/core/optimizers/sgd/sgd.hpp>
#include <mlpack/core/optimizers/adam/adam_update.hpp>

#include <mlpack/methods/ann/layer/layer.hpp>
#include <mlpack/methods/ann/rnn.hpp>
#include <mlpack/methods/ann/layer/lstm.hpp>
#include <mlpack/methods/ann/loss_functions/kl_divergence.hpp>
#include <mlpack/methods/ann/loss_functions/mean_squared_error.hpp>
#include <mlpack/methods/ann/loss_functions/cross_entropy_error.hpp>
#include <mlpack/prereqs.hpp>

using namespace mlpack;
using namespace mlpack::ann;
using namespace mlpack::optimization;

using namespace arma;
using namespace std;

// Prepare input of sequence of notes for LSTM
arma::cube getTrainX(const mat& tempDataset, const unsigned int& sequence_length)
{
    const unsigned int num_notes = tempDataset.n_rows;	
    const unsigned int num_sequences = (num_notes / sequence_length) + 1;
    cube trainX = cube(1, num_sequences, sequence_length);	
    for (unsigned int i = 0; i < num_sequences; i++)
    {
	for (unsigned int j = 0; j < sequence_length; j++)
	{
		trainX(0,i,j) = tempDataset(i+j,0);
	}
    }
    return trainX;
 }

arma::mat getReal(const mat& tempDataset, const int& sequence_length)
{
    mat real = mat(1,tempDataset.n_rows - sequence_length);
    for (unsigned int i = sequence_length; i < tempDataset.n_rows; i++)
    {
	 real(0,i-sequence_length) = tempDataset(i,0);
    }
    return real;
}	

// Generate array with 1 in the indice of the note present at a time step
arma::cube getTrainY(const mat& tempDataset, const int& size_notes, const int& sequence_length)
{
    cube proba = cube(size_notes, tempDataset.n_rows - sequence_length, sequence_length, fill::zeros);
    for (unsigned int i = sequence_length; i < tempDataset.n_rows; i++)
    {
	int note = tempDataset.at(i,0);
	proba.tube(note,i-sequence_length).fill(1);
    }
    return proba;
}

arma::mat getNotes(const mat& proba)
{
    unsigned int num_notes = proba.n_cols;
    mat notes = mat(1, num_notes);
    for (unsigned int i = 0; i < num_notes; i++)
    {
        notes(0,i) = index_max(proba.col(i));
    }
    return notes;
}				   

 /**
 * Returns the accuracy (percentage of correct answers).
 */

double accuracy(arma::mat& predicted, const arma::mat& real)
{
    // Calculating how many predicted notes coincide with actual notes.
    size_t success = 0;
    for (size_t j = 0; j < predicted.n_cols; j++) {
        if (predicted(0,j) == std::round(real(0,j))) {
            ++success;
        }
    }
    
    // Calculating percentage of correctly predicted notes.
    return (double) success / (double)predicted.n_cols * 100.0;
}

void trainModel(RNN<MeanSquaredError<>>& model,
                const cube& trainX, const cube& trainY, const mat& real)
{
    // The solution is done in several approaches (CYCLES), so each approach
    // uses previous results as starting point and have a different optimizer
    // options (here the step size is different).
        			      
     // Number of iteration per cycle.
    constexpr int ITERATIONS_PER_CYCLE = 5;

    // Number of cycles.
    constexpr int CYCLES = 20;

    // Step size of an optimizer.
    constexpr double STEP_SIZE = 5e-10;

    // Number of data points in each iteration of SGD
    constexpr int BATCH_SIZE = 5;

    // Setting parameters Stochastic Gradient Descent (SGD) optimizer.
    SGD<AdamUpdate> optimizer(
                              // Step size of the optimizer.
                              STEP_SIZE,
                              // Batch size. Number of data points that are used in each iteration.
                              BATCH_SIZE,
                              // Max number of iterations
                              ITERATIONS_PER_CYCLE,
                              // Tolerance, used as a stopping condition. This small number
                              // means we never stop by this condition and continue to optimize
                              // up to reaching maximum of iterations.
                              1e-8,
    			      false,
    			      // Adam update policy.
    			      AdamUpdate(1e-8, 0.9, 0.999));
	
	
    // Cycles for monitoring the process of a solution.
    for (int i = 0; i <= CYCLES; i++)
    {
        // Train neural network. If this is the first iteration, weights are
        // random, using current values as starting point otherwise.
       
       	model.Train(trainX, trainY, optimizer);
       
        // Don't reset optimizer's parameters between cycles.
        optimizer.ResetPolicy() = false;
        
        cube predOut;
        // Getting predictions on training data points.
        model.Predict(trainX, predOut);

        // Calculating accuracy on training data points.
        mat pred = getNotes(predOut.slice(predOut.n_slices - 1));
	cout << "Predicted notes" << pred << endl;
	cout <<	 "Actual notes" << real << endl;    
        double trainAccuracy = accuracy(pred, real);       

        cout << i << " - accuracy = "<< trainAccuracy << "%," << endl;
        
    }
}

/**
 * Run the neural network model and predict the class for a
 * set of testing example
 */
void predictNotes(RNN<MeanSquaredError<>>& model,
                  const unsigned int sequence_length, const unsigned int size_notes, const unsigned int size_music)
{
    
    cube start = cube(1, 1, sequence_length); // we initialize generation with a sequence of random notes
    for (unsigned int i = 0; i < sequence_length; i++)
    {
	start(0,0,i) = rand() % size_notes + 1; // random integer between 1 and size_notes
    }
	
    mat music = mat(size_music,1, fill::zeros);	
    cube compose;
    for (unsigned int i = 0; i < size_music; i = i + sequence_length)	
    {	    
    	// Getting predictions after starting notes .
    	model.Predict(start, compose);
	cout << compose << endl;    
    	// Fetching the notes from probability vector generated.
    	mat notes = getNotes(compose.slice(compose.n_slices - 1));
	    
    	for (unsigned int j = 0; j < sequence_length; j++)
	{
		int note = notes(0,j);
		music(i+j,0) = note;
		start(0,0,j) = note; // update start to continue generation
	}

	
    }
    cout << "Saving predicted notes to \"sonata.csv\" ..." << endl;

    // Saving results into Kaggle compatibe CSV file.
    data::Save("sonata.csv", music); 
    cout << "Music saved to \"sonata.csv\"" << endl;
}

int main () {
    
    cout << "Reading data ..." << endl; 
	
    mat tempDataset;
    const int rho = 3;

    data::Load("../utils/training.csv", tempDataset, true); // read data from this csv file, creates arma matrix with loaded data in tempDataset
    
   
    const int size_notes = max(tempDataset.row(0)) + 1;
    const int sequence_length = rho;
    const int size_music = 21;
	
    cube trainX = getTrainX(tempDataset, sequence_length);
    //cube trainY = getTrainY(tempDataset, sequence_length);
    cube trainY = getTrainY(tempDataset, size_notes, sequence_length);
    mat real = getReal(tempDataset, sequence_length);	
    cout << trainX << trainY << endl;
	
    RNN<MeanSquaredError<> > model(rho);
    model.Add<Linear <> > (trainX.n_rows, rho);
    model.Add<LSTM <> > (rho,512);
    model.Add<Linear <> > (512, 512);
    model.Add<LSTM <> > (512, 512);
    model.Add<Linear <> > (512, 256);
    model.Add<Dropout <> > (0.3);
    model.Add<Linear <> > (256, size_notes);
    //model.Add<SigmoidLayer <> >();
    model.Add<LogSoftMax<> > ();
    	
    cout << "Training ..." << endl;
    trainModel(model, trainX, trainY, real);
    
    cout << "Composing ..." << endl;
    predictNotes(model, sequence_length,size_notes, size_music);
    cout << "Finished :)" << endl;
    cout << "Saving model ..." << endl;
    // data::Save("mozart.xml", "mozart", model, false);
    cout << "Saved!" << endl;
    return 0;
}
