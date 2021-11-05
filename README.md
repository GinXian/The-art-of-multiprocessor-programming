# The-art-of-multiprocessor-programming

### Purpose

Just a repository convenient for reviewing and retrieval practice.

### The architecture of this repository(Hierarchy Descending Order)

1. folders for all relative chapters
2. explanation file(s), source codes for implementations and tests.

### Retrieval Practice Method

#### What is the schedule?

The schedule is formed as below:

1. Every commit will be a review to a specific chapter, and it will be tagged with the date. If, even rare, there are more than one commit in a day, each commit must be strictly classified into chapters and share the same date tag. Each commit will modify all files in the folder of a specific chapter. Therefore, I can use `git diff` to check the correctness.
2. Regarding a specific chapter, it will be reviewed once after a specific amount of days since the last review. The amount will grow exponentially from 1 to 16. After it arrives at 16, growing is stopped. When the amount is still growing, the content that need reviewing is considered **unstable**, and otherwise **stable**. Therefore, it means that **stable** contents will be reviewed on a uniform basis.