# IOS Project Vol. 2: Semaphores
The second project I did for my Operating Systems class on BUT FIT. The task is inspired by a molecule synchronization problem from Allen B. Downey's "The Little Book of Semaphores".
One of the best and most fun projects in the uni. Final score: 15/15.

## How to run: 

```
make
./proj2 NO NH TI TB
```

- **NO:** Number of oxygens
- **NH:** Number of hydrogens
- **TI:** Maximal time in milliseconds, for which the every atom can wait. After that, it travels to the "queue" and either takes part in creating an H<sub>2</sub>O molecule. 0<=TI<=1000
- **TB:** Maximal time in milliseconds needed for one H<sub>2</sub>O molecule creation. 0<=TB<=1000

## Few notes:
- No active waiting was used. The synchronization is all done with POSIX semaphores and shared memory.
- While the book may mention priority queue, or it might be an interesting idea to implement it, it was not done, simply because it wasn't needed in the task. It is therefore possible, that oxygen 4 can surpass oxygen 1 and create a molecule, while the oxygen 1 dies without creating.
- While creating the molecule, the first two atoms will always wait for the slowest one, therefore it is not possible for one atom to print **"molecule created"**, before the others print **"creating molecule"**.
- However, the molecules will know, if they won't be able to create a molecule and due to my implementation, they will exit as soon as possible. While it might not be clean and would look better, if these atoms waited for all molecules to create, the task did not specify any of those details and me trying to implement it caused more harm than good (deadlocks).
