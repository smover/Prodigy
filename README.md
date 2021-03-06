# Prodigy

## Project description:

Our aim is to train a machine learning algorithm for music composition. For this, we will use an existing LSTM library on C++ that we will use to train. Training will be done using text files, so we will also need to write an algorithm to convert MIDI files into usable text.

For the first 3 weeks of the project, we divided ourselves into two teams.

The first team, "Translators", will define how music will be represented by text: what notes are mapped to which characters, how time/ticks are defined through text... They will also write an algorithm who takes as input MIDI files and outputs a text file with relevant elements in defined format.

The second team, "Builders", will construct the AI used for training. They will grasp the theory and functioning of neural networks, determine which library to use, understand how to use it for music generation, and from the third week start training using translated MIDI files of Mozart's music.

