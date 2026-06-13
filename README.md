# Replicating: Enabling ECN for Datacenter Networks with RTT Variations

**Team Members:**  
Stanislaw Ostyk-Narbutt (stanislaw.ostyknarbutt@mail.polimi.it)
FirstName LastName2 (email address);  
FirstName LastName3 (email address)

---

**Source Paper:**
Junxue Zhang, Wei Bai, and Kai Chen. 2019. Enabling ECN for Datacen-
ter Networks with RTT Variations. In The 15th International Conference
on emerging Networking EXperiments and Technologies (CoNEXT ’19), De-
cember 9–12, 2019, Orlando, FL, USA. ACM, New York, NY, USA, 13 pages.
https://doi.org/10.1145/3359989.3365426


**Project:**
All slurm cluster scripts, python plotting files, and modified NS3 topology files are in the following repo:
[github.com/ElBi21/NetworkComputing](github.com/ElBi21/NetworkComputing)

It is organized in folders by figure recreated (e.g. `fig2` for Figure 2, etc.) and `fat-tree` contains the new extension we added to the paper.

---

# 1. Introduction

<!-- Introduce the paper by summarizing:

- The problem the paper addresses and its importance
- The key ideas behind its solution and its approach
- The main contributions -->



# 2. Selected Result

<!-- Mention which result of the paper you are reproducing, and explain its importance.

For example:

> “Figure 1 shows that method A improves throughput by 35% over method B under workload *W*. This experiment shows that paper can effectively overcome the motivated challenge.”

<center>
  <img
    alt="The figure shows that method A improves throughput compared to method B"
    src="figures/one_bar.png"
    style="width:30%;"
    />
  <p>Figure 1: The figure shows that method A improves throughput compared to method B</p>
</center> -->

# 3. Environment Setup

*Note:* This section should contain enough information to allow someone else to
reproduce *your* report. Share hardware and/or software setup relevant to your
experiment. For example:

**Hardware Environment:**
CPU, Memory, Storage, Network, Cloud / local / cluster, Any relevant micro-architectural details

**Software Environment**
OS version, Kernel version, Compiler version, Libraries, Dependencies, Paper artifact used (Yes/No; version/commit hash)

**Configuration Parameters:**

- Workload configuration
- Dataset
- Runtime parameters and flags

**Deviations from the Original Setup:**

Clearly describe any difference between papers and your experiment environment.

- Hardware differences
- Software version differences
- Dataset substitutions
- Unavailable components

Explain why these deviations were necessary.

If something was **missing in the original paper**, state it. For example:

> The paper does not specify X. We assumed Y (or explored range *a* to *b*).

# 4. Experiment Result

<!-- > Explain how your experiment was conducted and then what results you acquired. 
Afterwards, compare your results with those of the paper and state your
takeaways.

Step-by-step description:

1. Execution procedure
1. Measurement method
1. Number of runs
1. Statistical treatment (mean, median, CI, etc.)

Also Describe:

- How did you ensure correctness (did you check also other metrics to make sure the experiment is running correctly?)
- Did you do any debugging? Discuss issues you faced and how you overcame them (if applicable consider allocating a subsection for this item) 

Share your result and compare them with the paper's. Then discuss your takeaways.

For comparison include:

- Graph(s) or table(s)
- Matching axes and units with the source paper
- Error bars if applicable
- You may want to report difference with the original results (e.g., absolute
number or percentage).

For example: -->

<!-- <center>
  <div style="display:inline-block; width:30%;">
    <img
      alt="The figure shows that method A improves throughput compared to method B"
      src="figures/one_bar.png"
      style="width:100%"
      />
    <p>Figure 2: The figure shows that method A improves throughput compared to method B</p>
  </div>
  <div style="display:inline-block; width:30%; padding-left: 1em">
    <img
      alt="Our reproduction of Figure 1 shows results with the similar trend as claimed by the paper"
      src="figures/two_bar.png"
      style="width:100%"
      />
    <p>Figure 3: Our reproduction of Figure 1 shows results with the similar trend as claimed by the paper</p>
  </div>
</center> -->

<!-- > **Reminder:** the goal is not achieve the exact results of the paper, but to do a rigorous experiment with similar assumptions from the source paper and gain insight. The insight can be correctness of work, failure to reproduce same results, or even infeasibility of doing such experiment for interesting reasons. -->

We replicated several of the results of the paper in order to assess its various claims about ECN#'s performance. Each is documented below.

### Recreated Figure 2, 'Instantaneous Marking cannot achieve high throughput and low latency simultaneously`

<center>
  <div style="display:inline-block; width:30%; padding-left: 1em">
    <img
      alt="Figure 1: Our reproduction of Figure 2 on the simulated NS3 large-scale setup"
      src="figures/Recreated-Fig2.png"
      style="width:100%"
      />
    <p>Figure 1: Our reproduction of Figure 2 on the simulated NS3 large-scale setup</p>
  </div>
</center>


# 5. Further Exploration
<!-- 
In this project you are required to also explore a research question of your own. Either:

1. Take the same test with different input workload or a variation of a test that is not present in the paper and comment the results you obtain
1. Implement a new feature on top of the system you evaluated and show a figure showing the performance

Discuss which approach you take, and what you explored. Explain what was your
motivation and importance of your question. -->

For the extension, we wanted to test the effectiveness of NS3 on a new topology. The paper only demonstrates its effectiveness on a Leaf-Spine two-tier network topology with 128 servers, and we were curious if a different topology would show similar trends. We were particularly curious about Fat Trees because their three-tier architecture would add complexity to the network and provide more possible flow paths. Since we already saw from our experimentation on recreating Figure 2 that topology really matters and may lead to differing results, we were curious if ECN# would perform well. 

We dug into the code to find where the topology is defined, which is in `examples/rtt-variations/large-scale.cc`. Here, the NS3 topology is defined. 


## 5.1. Methodology and Result

<!-- Report the experiment you designed for answering the question and share the
result you got.

Include:

- Graph(s) or table(s)
- How the experiment was conducted (share the details)
- What did you discover? -->

<center>
  <div style="display:inline-block; width:30%; padding-left: 1em">
    <img
      alt="Figure 1: Our reproduction of Figure 2 on the simulated NS3 large-scale setup"
      src="figures/Fat-Tree-Experiment.png"
      style="width:100%"
      />
    <p>Figure 2: ECN# vs TCN Performance Comparison on Fat Tree with K=4</p>
  </div>
</center>

# 6. Reproducibility Assessment of the Paper

<!-- Evaluate the paper itself:

- Was the methodology clearly described?
- Was the artifact usable?
- How difficult was reproduction? -->

Part of the paper was very well documented and reproducible, as the authors provide a Docker image with the preconfigured NS3 simulation to run certain experiments. However, this only pertains to the large scale testing they did, all earlier graphs, including Figure 2 as an example, required us to experiment with NS3 ourselves in order to try and replicate. The code itself was well written, although it did contain some typos even in the command line interface, but overall the authors did a great job with ensuring reproducability. 

# 7. Conclusion

Conclude the report by mentioning the takeaways of experiments you did


---

# Appendix

You are asked to write this report using Markdown. You can find a cheat sheet
of Markdown syntax at this [link](https://rust-lang.github.io/mdBook/format/markdown.html).

For generating a PDF file from your report you can use a tool of your choice.
*md2pdf* is one such tool. See this [link](https://pypi.org/project/md2pdf/)
for more information about it. You can also use an online editor such as [this](https://www.md2pdf.io/).

