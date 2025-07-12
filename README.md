# MDBPE_TF

<table>
  <tr>
    <td width="50%">
      Repository of the Paper <a href=https://arxiv.org/abs/2411.10281>"Multidimensional Byte Pair Encoding: Shortened Sequences for Improved Visual Data Generation" (https://arxiv.org/abs/2411.10281)</a>.<br/>
<br/>
Our algorithm compresses visual data in order to make tasks like generation more efficient: Shorter sequences, even if they are from a larger vocabulary, are easier to handle for deep learning architectures like transformers. The images show representative examples after the same training time, with training on shortened sequences (right) producing better results faster.<br/>
<br/>
This repository contains two parts:<br/>
      -"generator.ipynb": Our code applied to MNIST, with both tokenisation and generation (to learn what we're doing). Just copy-paste & execute<br/>
      -"tokenprocessor": our pre-processor, the core of our work. Produces a multidimensional byte pair encoding, either from a fast C++ implementation (recommended for anything more than 12-pixel-MNIST) or a Python version<br/><br/><br/>
	  If you want to generate your own data, 1. Apply MDBPE (either python or C++ version), 2. Use the transformer from the demo file, should be self explanatory :)<br/>
    <b>If you do like your sanity, when playing around with this, only use a few hundred files to get things running! Debugging on hardcore-overfitting is a lot better for your mind ;)</b>
    </td>
    <td width="50%">
      <img src="https://github.com/user-attachments/assets/fdde38d7-d3ed-4968-91b0-a4acbfe62996" alt="image" width="400px">
    </td>
  </tr>
</table>
