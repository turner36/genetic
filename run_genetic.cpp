/*******************************************************************************
 * FILE NAME: run_genetic.cpp                                                  *
 * AUTHOR: Summer Turner                                                       *
 * LAST DATE MOTIFIED: 23 October 2017                                         *
 *                                                                             *
 * PURPOSE: The point of this program is to illustrate how an algorithm can    *
 * modify its behavior by learning and adapting during evolution. The          *
 * programmer defines the evolution rules, and let's the program itself        *
 * figure out an optimal algorithm.                                            * *                                                                             *
 * Robby has no other knowledge of the system than his current context,        *
 * i.e. he does not know how large the system is, what shape it is, or any     *
 * other information except his current context.                               *
 *                                                                             *
 * NOTES:                                                                      *
 * Max theoretical score: 500 (~50 cans x 10 points)                           *
 * Min theoretical score: -5 * STEPS  = -1000 with 200 steps                   *
 *                                                                             *
 * MODIFICATIONS:                                                              *
 * 1. Fixed format to follow NASA C style standards.                           *
 * 2. Changed the evolution selection from elitism to tournament selection.    *
 * 3. Winning strategy is written to array.txt to communicate with the second  *
 *    program benchmarkWinningStrategy.                                        *
 *                                                                             *
 * REFERENCES:                                                                 *
 * [1] tolex3. “genetic”. GitHub, 14 March 2012,                               *
 *     https://github.com/tolex3/genetic. 23 October 2017.                     *
 ******************************************************************************/

#include <stdio.h>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <sys/time.h>
#include "constants.h"
#include "context.h"
#include "robby.h"
#include "strategy.h"
#include "strategyStore.h"

using namespace std;

Strategy s;
StrategyStore st;

/******************************************************************************/
/* Main function. */
int main(int argc, char **argv) {

    int genCounter = 0;
    int populationCount = 0;
    int sessionScores = 0;
    int fieldMatrix[10][10];
    int c;
    int nr_generations = GENERATIONS;
    int nr_steps = STEPS;

    while (1)
    {
        static struct option long_options[] =
        {
//          {"Number of agents",                    required_argument, 0, 'a'},
//          {"Number of cleaning sessions",         required_argument, 0, 'c'},
            {"Number of generations",               required_argument, 0, 'g'},
//          {"Number of survivors",                 required_argument, 0, 'n'},
            {"Number of steps in cleaning session", required_argument, 0, 's'},
            {0, 0, 0, 0}
        };

        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "g:s:", long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c)
        {
            case 0:
                printf ("option %s", long_options[option_index].name);
                if (optarg)
                    printf (" with arg %s", optarg);
                printf ("\n");
                break;
            case 'g':
                printf ("option -%c with value `%s'\n", c, optarg);
                nr_generations = (int) atol(optarg);
                break;
            case 's':
                printf ("option -%c with value `%s'\n", c, optarg);
                nr_steps = (int) atol(optarg);
                break;
            default:
                abort ();
        }
    }

    srand(time(NULL));

    /**************************************************************************/
    /* Initialize agents. */
    Robby agentArray [NR_AGENTS];

    for (int agent = 0; agent < NR_AGENTS; agent++)
    {
        s.setRandomStrategy();
        agentArray[agent].setStrategy(s);
    }

    /**************************************************************************/
    /* While not finished with GENERATIONS. */
    while (genCounter < nr_generations)
    {
        for (int agent = 0; agent < NR_AGENTS; agent++)
        {
            agentArray[agent].resetStatistics();

            /* All sessions for an agent. */
            for (int session = 0; session < SESSIONS; session++)
            {
                agentArray[agent].setPos(0, 0);
                agentArray[agent].initializeField(fieldMatrix);
                agentArray[agent].resetStatistics();

                for (int steps = 0; steps < nr_steps; steps++)
                {
                    /* Make one step by figuring out current context, getting
                     * the index for that context, getting the action for that
                     * index. */
                    agentArray[agent].updateContext();
                    agentArray[agent].makeMove(agentArray[agent].getStrategy().getAction(agentArray[agent].getContext().getCoding()), false);
                }

                /* Register score for this session. */
                sessionScores  += agentArray[agent].getPoints();
            }

            int sessionAvg = 0;
            sessionAvg = sessionScores / SESSIONS;
            sessionScores = 0;

            agentArray[agent].getStrategy().updateScore(sessionAvg);
        }

        for (int i = 0; i < NR_AGENTS; i++)
        {
            if (! (st.exists(agentArray[i].getStrategy())))
                st.addStrategy(agentArray[i].getStrategy());
        }

        /* Create child population. */
        populationCount = 0;
        Strategy survivors[SURVIVORS];

        for (int performers = 0; performers < SURVIVORS; performers++)
        {
            st.getOne(survivors[performers], performers);
            agentArray[performers].setStrategy(survivors[performers]);

            if (rand() % 100 == 0)
            {
                agentArray[populationCount].getStrategy().mutate();
                agentArray[populationCount].getStrategy().updateMutationCount();
            }

            populationCount++;
        }

        /* While not enough in the child population. */
        while (populationCount < NR_AGENTS)
        {
            /* Modified to use tournament selection.
             * Select 3 individuals from the population at random and select the
             * best out of these to become a parent. Repeat for next parent. */
            int best = 0;
            for (int i = 0; i < 3; i++)
            {
                int index = rand() % SURVIVORS;
                if ((best == 0) || survivors[index].getScore() > survivors[best].getScore())
                    best = index;

            }
            int father = best;

            best = 0;
            for (int j = 0; j < 3; j++)
            {
                int index = rand() % SURVIVORS;
                if (!(best == father))
                    if ((best == 0) || survivors[index].getScore() > survivors[best].getScore())
                        best = index;

            }
            int mother = best;

            Strategy *temp;

            /* First child for one parent pair. */
            temp = copulate(survivors[father], survivors[mother]);
            agentArray[populationCount].setStrategy(*temp);
            agentArray[populationCount].getStrategy().setBirthGeneration(genCounter);
            delete temp;

            /* Second child for one parent pair. */
            temp = copulate(survivors[mother], survivors[father]);
            agentArray[populationCount].setStrategy(*temp);
            agentArray[populationCount].getStrategy().setBirthGeneration(genCounter);
            delete temp;

            /* Mutate a fraction of the children. */
            if (rand() % 4 == 0)
            {
                agentArray[populationCount].getStrategy().mutate();
                agentArray[populationCount].getStrategy().updateMutationCount();
            }

            /* Decrease the likelihood of lower performing parents to breed. */
            if (rand() % 10 == 0)
            {
                father += 2;
                mother += 2;
            }

            populationCount++;
        }

        if (genCounter % 10 == 0)
        {
            cout << "GENERATIONS " << genCounter << "-" << genCounter + 9 << endl;
            st.printStore();
            cout << endl << "generation average similarity: " << st.averageSimilarity() << endl;
            cout << endl << "**********************************************" << endl;
        }

        if (genCounter < nr_generations - 1)
            st.resetRanks();

        genCounter++;
    }

    cout << endl << "FINAL RANKINGS after " << genCounter << " generations " << endl;
    st.printStore();

    /**************************************************************************/
    /* Testrun the winning strategy for same number of sessions as the
     * generations did. */
    Robby winningAgent;
    Strategy winner;
    st.getOne(winner, 0);
    int winnerScore = 0;

    winningAgent.setStrategy(winner);
    winningAgent.getStrategy().saveStrategy();
    winningAgent.getStrategy().printStrategy();

    cout << endl << endl
         << "WINNERS RANK SCORE: " << winner.getScore()
         << " BIRTHGEN: " << winner.getBirthGeneration()
         << " MUTATIONS: " << winner.getMutationCount() << endl;

    for (int nr_sessions = 0; nr_sessions < SESSIONS; nr_sessions++)
    {
        cout << endl << "Starting session: " << nr_sessions;
        winningAgent.setPos(0, 0);
        winningAgent.initializeField(fieldMatrix);
        winningAgent.resetStatistics();

        for (int steps = 0; steps < nr_steps; steps++)
        {
            winningAgent.updateContext();
            winningAgent.makeMove(winningAgent.getStrategy().getAction(winningAgent.getContext().getCoding()), false);
        }

        winnerScore += winningAgent.getPoints();
    }

    winnerScore = winnerScore / SESSIONS;
    cout << endl << "WINNER'S testrun average score: " << winnerScore << endl;
}
