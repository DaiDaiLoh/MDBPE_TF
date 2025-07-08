# MDBPE_TF

<table>
  <tr>
    <td width="50%">
      Repository of the Paper <a href=https://arxiv.org/abs/2411.10281>"Multidimensional Byte Pair Encoding: Shortened Sequences for Improved Visual Data Generation" (https://arxiv.org/abs/2411.10281)</a>.<br/>
<br/>
Our algorithm compresses visual data in order to make tasks like generation more efficient: Shorter sequences, even if they are from a larger vocabulary, are easier to handle for deep learning architectures like transformers. The images show representative examples after the same training time, with training on shortened sequences (right) producing better results faster.<br/>
<br/>
This repository contains three parts:<br/>
      -"demo": a (small) MNIST demo with both tokenisation and generation (to learn what we're doing).<br/>
      -"mdbpe": a fast C++ implementation of multidimensional byte pair encoding encoding (recommended for anything more than MNIST)<br/>
      -"generation": our full implementation for generation of the byte pair encoded elements<br/>
    </td>
    <td width="50%">
      <img src="https://github.com/user-attachments/assets/fdde38d7-d3ed-4968-91b0-a4acbfe62996" alt="image" width="400px">
    </td>
  </tr>
</table>
