#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpich/mpi.h>
#include <time.h>

void setElections(int voters , int candidates , int** elections)
{
    int* gen= malloc(candidates * sizeof(int));
    memset(gen,0,candidates*sizeof(int));
    int  i=0,j=0;
    int rand1 = 0 ;

    srand(time(NULL));
    for(i=0 ; i<voters ; i++)
    {
        memset(gen,0,candidates*sizeof(int));
        for(j=0 ; j<candidates ; j++)
            {
               rand1 = rand()%candidates+1;
                if(gen[rand1-1] == 1){
                j--;}
                else {
                   elections[i][j] = rand1;
                    gen[rand1-1]=1;
                }
            }
    }
}

void writeToFile(FILE *fptr,int voters , int candidates , int** elections){
    int i=0,j=0;
    fprintf(fptr,"%d\n%d\n",candidates , voters);
    for(i=0 ; i<voters ; i++)//write to file
    {
        for(j=0 ; j<candidates ; j++)
            {
                if(j== candidates-1) fprintf(fptr,"%d",elections[i][j]);
                else fprintf(fptr,"%d ",elections[i][j]);
            }
            fprintf(fptr,"\n");
    }
    //fclose(fptr);
}
int main(int argc , char * argv[])
{
    int candidates ,voters;
    int i=0,j=0;
    FILE *fptr;
    int **elections = (int **)malloc(voters * sizeof(int*));
    float* candidate= malloc(candidates * sizeof(float));
    memset(candidate,0,candidates*sizeof(float));

    int my_rank;
    int p,portion = 0,rem=0;
    MPI_Status status;
    MPI_Init( &argc , &argv );
    int fileSize;

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    MPI_Comm_size(MPI_COMM_WORLD, &p);

    int choice = 3 ;
    char fname[20];

    if(my_rank == 0){

        printf("Enter 0 for creating a new file or 1 to calculate the votes and show winner\n");
        scanf("%d",&choice);
        if(choice == 0){
            printf("Enter the file name: \n");
            scanf("%s",fname);

            printf("Enter num of candidates: \n");
            scanf("%d",&candidates);/////////////////////////////////

            printf("Enter num of voters: \n");
            scanf("%d",&voters);/////////////////////////////////

            elections = (int **)malloc(voters * sizeof(int*));
             for(i = 0; i < voters; i++)
                elections[i] = (int *)malloc(candidates * sizeof(int));

            setElections(voters , candidates ,elections);//////////////////////////////

            if ((fptr = fopen(fname,"w")) == NULL){
            printf("Error! opening file");
            exit(1);
            }
            writeToFile(fptr,voters , candidates, elections);////////////////////////////
            fclose(fptr);
            printf("File Created!\n");
            return 0;
        }
    else if(choice == 1){
        printf("Enter the file name: \n");
        scanf("%s",fname);

        if ((fptr = fopen(fname,"r")) == NULL){
            printf("Error! opening file");
            exit(1);
            }
              fseek(fptr, 0, SEEK_END);
            if(ftell(fptr) == 0 ) { printf("the file is empty! \n");return 0;}
       //read
        rewind(fptr);

        fscanf(fptr,"%d%d",&candidates,&voters);

       //setting portion
       printf("%d %d\n",voters , candidates);
       portion = voters/p;
       rem = voters%p;
    }
}
        //broadcast portion & candidates
        MPI_Bcast(&portion, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&candidates, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&voters, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(fname, 20, MPI_CHAR, 0, MPI_COMM_WORLD);

       int** localPref = (int **)malloc((portion+rem) * sizeof(int*));
       for(i = 0; i < portion+rem; i++)
           localPref[i] = (int *)malloc(candidates * sizeof(int));

        if(my_rank != 0)
        {
            if ((fptr = fopen(fname,"r")) == NULL){
            printf("Error! opening file");
            exit(1);
            }
            fseek(fptr,4,SEEK_SET);
            fseek( fptr, my_rank*candidates*2*portion,SEEK_CUR);
        }
        //read
        for(i=0 ; i<portion ; i++)
        {
            for(j=0 ; j<candidates ; j++)
            {
                fscanf(fptr,"%d",&localPref[i][j]);
                //printf("process num: %d - %d \n",my_rank,localPref[i][j]);
            }
           // printf("\n");
        }
        //Handling the reminders
        if(my_rank == 0 && rem !=0){
            fseek(fptr,4,SEEK_SET);
            fseek( fptr, p*candidates*2*portion,SEEK_CUR);
            for(i=portion ; i<rem+portion ; i++){
            for(j=0 ; j<candidates ; j++)
            {
                fscanf(fptr,"%d",&localPref[i][j]);
                //printf("process num: %d - %d \n",my_rank,localPref[i][j]);
            }
           // printf("\n");
        }
    }
        fclose(fptr);

/***********************************************************************************/
        float* localCandidate= malloc(candidates * sizeof(float));
        memset(localCandidate,0.00000,candidates*sizeof(float));

        for(i=0 ; i<portion ; i++)
        {
            int x = localPref[i][0];
            localCandidate[x-1] +=  (1.0/voters)*100.0;
        }
        if(my_rank == 0 && rem != 0){

        for(i=portion ; i<rem+portion ; i++)
        {
            int x = localPref[i][0];
            localCandidate[x-1] +=  (1.0/voters)*100.0;
        }
        }
/*****************************************************************************************/
        //reducing to master Adding the calculated votes
        for(i=0 ; i<candidates;i++){
            MPI_Reduce(&localCandidate[i], &candidate[i], 1, MPI_FLOAT, MPI_SUM, 0,MPI_COMM_WORLD);
        }

        int check = 0;float max =0.0,max2=0.0;int candidateNum1=0,candidateNum2=0;
        if(my_rank == 0){
                printf("\n********************************ROUND1*************************************\n");
            for(i=0 ; i<candidates ; i++)
            {
                printf("--Candidate num: %d got %f percent of all votes\n",i+1,candidate[i]);
            }
            printf("\n");
            for(i=0 ; i<candidates ; i++)
            {
                if(candidate[i] > max){
                    max2=max;
                    candidateNum2 = candidateNum1;
                    max=candidate[i];
                    candidateNum1 = i;
                }
                else if(candidate[i] == max || (candidate[i]<max && candidate[i]>max2) ){
                        max2 = candidate[i];
                        candidateNum2 = i;
                }
            }
            if( max>50.00){printf("--candidate number %d won in round 1\n",candidateNum1+1);return 0;}
            else if(max<=50.00) {
                   check=1; printf("--candidate number %d and candidate number %d are going to round 2\n",candidateNum1+1,candidateNum2+1);
                   candidateNum1++;candidateNum2++;
                  printf("\n/********************************ROUND2***************************************/\n\n");
            }
        }
        /******************************************ROUND2**************************************************/
        MPI_Bcast(&check, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if(check == 1){
        MPI_Bcast(&candidateNum1, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&candidateNum2, 1, MPI_INT, 0, MPI_COMM_WORLD);
    //updating preference for round 2
       int** localPrefR2 = (int **)malloc((portion+rem) * sizeof(int*));
       for(i = 0; i < portion+rem; i++)
           localPrefR2[i] = (int *)malloc(2 * sizeof(int));

       for(i=0 ; i<portion ; i++)
       {
           j=0;
           while(1)
           {
               if(localPref[i][j] == candidateNum1) {
                    localPrefR2[i][0] = candidateNum1;
                    localPrefR2[i][1] = candidateNum2;
                    break;
               }
               if(localPref[i][j] == candidateNum2) {
                    localPrefR2[i][0] = candidateNum2;
                    localPrefR2[i][1] = candidateNum1;
                    break;
               }

               j++;
           }
       }
    if(my_rank == 0 && rem!=0){
           for(i=portion ; i<portion+rem ; i++)
           {
               j=0;
               while(1)
               {
                   if(localPref[i][j] == candidateNum1) {
                        localPrefR2[i][0] = candidateNum1;
                        localPrefR2[i][1] = candidateNum2;
                             break;
                   }
                   if(localPref[i][j] == candidateNum2) {
                        localPrefR2[i][0] = candidateNum2;
                        localPrefR2[i][1] = candidateNum1;
                        break;
                   }

                   j++;
               }
           }
    }
/*********************************************************************************/
        float* localCandidateR2= malloc(2 * sizeof(float));
        memset(localCandidateR2,0.00,2*sizeof(float));

        for(i=0 ; i<portion ; i++)
        {
            int x = localPrefR2[i][0];
            if(x == candidateNum1) localCandidateR2[0] +=  (1.0/voters)*100.0;
            if(x == candidateNum2) localCandidateR2[1] +=  (1.0/voters)*100.0;
        }
        if(my_rank == 0 && rem!=0){

        for(i=portion ; i<rem+portion ; i++)
        {
            int x = localPrefR2[i][0];
             if(x == candidateNum1) localCandidateR2[0] +=  (1.0/voters)*100.0;
            if(x == candidateNum2) localCandidateR2[1] +=  (1.0/voters)*100.0;
        }
        }
/**********************************************************************************/
    float* candidateR2= malloc(2* sizeof(float));
    memset(candidateR2,0,2*sizeof(float));

        //reducing
        for(i=0 ; i<candidates;i++){
            MPI_Reduce(&localCandidateR2[i], &candidateR2[i], 1, MPI_FLOAT, MPI_SUM, 0,MPI_COMM_WORLD);
        }
    if(my_rank == 0){

            printf("--Candidate num: %d got %f percent of all votes\n",candidateNum1,candidateR2[0]);
            printf("\n");
            printf("--Candidate num: %d got %f percent of all votes\n\n",candidateNum2,candidateR2[1]);

            if(candidateR2[0] > candidateR2[1]){
                printf("candidate number %d won in round 2\n",candidateNum1);
            }

            if(candidateR2[0] < candidateR2[1]){
                printf("--candidate number %d won in round 2\n",candidateNum2);
            }
    }
    }
        free(*elections);
        free(elections);
        free(*localPref);
        free(localPref);
        free(localCandidate);
        free(candidate);
        MPI_Finalize();
    return 0;
}
