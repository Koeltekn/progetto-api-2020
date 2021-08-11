#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAXSTRLEN 1024
#define MAXADDRESSLENGTH 10

//Text rows
typedef struct tTextNode
{
    char *content;
    struct tTextNode *next;
    struct tTextNode *prev;
} tTextNode;

//List of commands
typedef struct tCommandStackNode
{
    tTextNode *sublistHead;
    tTextNode *sublistTail;
    //c for change, d for delete, n for changes with no effect
    char comType;
    //For delete commands, it's the nodes removed, for change commands, it's the nodes changed
    int nodesBefore;
    //For delete commands, it's 0, for change commands, it's the changed nodes plus the added nodes
    int nodesAfter;
    struct tCommandStackNode *next;
} tCommandStackNode;

tTextNode *goToRow(tTextNode *, int, int);
void addTextNode(tTextNode **, char[], int *);
void pushCommandStack(tCommandStackNode **, tCommandStackNode *);
tCommandStackNode *popCommandStack(tCommandStackNode **);
void cutSublist(tTextNode **, tTextNode **);
void pasteSublist(tTextNode **, tTextNode **);
void printText(tTextNode *, int, int, int);
void changeText(tTextNode **, tCommandStackNode **, int, int, int *);
void deleteText(tTextNode **, tCommandStackNode **, int, int, int *);
void undoText(tTextNode **, tCommandStackNode *, int *);
void redoText(tTextNode **, tCommandStackNode *, int *);
void undoRedo(tTextNode **, tCommandStackNode **, tCommandStackNode **, int *, int *, int *);

tTextNode *goToRow(tTextNode *sentinelNode, int rowNum, int textLength)
{
    if (rowNum > 0 && rowNum <= textLength)
    {
        if (rowNum <= (textLength / 2))
        {
            //Go forward
            for (int i = 1; i <= rowNum; i++, sentinelNode = sentinelNode->next)
                ;
            return sentinelNode;
        }
        else
        {
            //Go backwards
            for (int i = textLength; i >= rowNum; i--, sentinelNode = sentinelNode->prev)
                ;
            return sentinelNode;
        }
    }
    else
    {
        return NULL;
    }
}

void addTextNode(tTextNode **prevNode, char content[], int *textLength)
{
    tTextNode *newNode = malloc(sizeof(tTextNode));
    newNode->content = malloc(strlen(content) + 1);
    strcpy(newNode->content, content);
    newNode->next = (*prevNode)->next;
    newNode->prev = (*prevNode);
    (*prevNode)->next->prev = (newNode);
    (*prevNode)->next = newNode;
    (*textLength)++;
}

void pushCommandStack(tCommandStackNode **stackHead, tCommandStackNode *newEle)
{
    newEle->next = (*stackHead);
    (*stackHead) = newEle;
}

tCommandStackNode *popCommandStack(tCommandStackNode **stackHead)
{
    tCommandStackNode *elem = (*stackHead);
    (*stackHead) = (*stackHead)->next;
    elem->next = NULL;
    return elem;
}

void cutSublist(tTextNode **sublistHead, tTextNode **sublistTail)
{
    (*sublistHead)->prev->next = (*sublistTail)->next;
    (*sublistTail)->next->prev = (*sublistHead)->prev;
}

void pasteSublist(tTextNode **sublistHead, tTextNode **sublistTail)
{
    (*sublistHead)->prev->next = (*sublistHead);
    (*sublistTail)->next->prev = (*sublistTail);
}

void printText(tTextNode *sentinelNode, int ind1, int ind2, int textLen)
{
    if (ind1 == 0)
    {
        printf(".\n");
        ind1 = 1;
    }
    //Go to ind1
    tTextNode *start = goToRow(sentinelNode, ind1, textLen);
    //Print ind2-ind1+1 rows, or untill there are no rows left
    int i = ind1;
    if (start != NULL)
    {
        for (; (i < ind2 + 1) && (start->content != NULL); i++, start = start->next)
            printf("%s\n", start->content);
    }
    //Check if there are empty rows to print
    if (i < ind2 + 1)
        for (; i < ind2 + 1; i++)
            printf(".\n");
}

void changeText(tTextNode **sentinelNode, tCommandStackNode **undoStackHead, int ind1, int ind2, int *textLength)
{
    tCommandStackNode *newCommand = malloc(sizeof(tCommandStackNode));
    newCommand->next = NULL;
    newCommand->nodesAfter = 0;
    newCommand->nodesBefore = 0;
    newCommand->sublistHead = NULL;
    newCommand->sublistTail = NULL;
    tTextNode *start;
    tTextNode *end;
    char newText[MAXSTRLEN];
    if (ind1 <= *textLength)
    {
        newCommand->comType = 'c';
        //Cut sublist
        start = goToRow(*sentinelNode, ind1, *textLength);
        if (ind2 > *textLength)
        {
            ind2 = *textLength;
        }
        if (ind1 == ind2)
        {
            end = start;
        }
        else
        {
            end = goToRow(*sentinelNode, ind2, (*textLength));
        }
        //Save sublist for reattatchment in case of undo and redos
        newCommand->sublistHead = start;
        newCommand->sublistTail = end;
        newCommand->nodesBefore = (ind2 - ind1 + 1);
        cutSublist(&start, &end);
        start = start->prev;
        (*textLength) -= (ind2 - ind1 + 1);
    }
    else
    {
        //Add a node
        newCommand->comType = 'a';
        newCommand->nodesBefore = 0;
        start = (*sentinelNode)->prev;
    }
    int i = 0;
    fgets(newText, sizeof(newText), stdin);
    *(strchr(newText, '\n')) = '\0';
    while (strcmp(newText, ".") != 0)
    {
        addTextNode(&start, newText, textLength);
        if (newCommand->sublistHead == NULL)
        {
            newCommand->sublistHead = start->next;
        }
        i++;
        start = start->next;
        fgets(newText, sizeof(newText), stdin);
        *(strchr(newText, '\n')) = '\0';
    }
    if (newCommand->sublistTail == NULL)
    {
        newCommand->sublistTail = start;
    }
    newCommand->nodesAfter = i;
    pushCommandStack(undoStackHead, newCommand);
}

void deleteText(tTextNode **sentinelNode, tCommandStackNode **undoStackHead, int ind1, int ind2, int *textLength)
{
    tCommandStackNode *newCommand = malloc(sizeof(tCommandStackNode));
    newCommand->next = NULL;
    newCommand->nodesAfter = 0;
    newCommand->nodesBefore = 0;
    newCommand->sublistHead = NULL;
    newCommand->sublistTail = NULL;
    tTextNode *start;
    tTextNode *end;
    if (ind1 <= *textLength)
    {
        start = goToRow(*sentinelNode, ind1, *textLength);
        if (ind2 > *textLength)
        {
            ind2 = *textLength;
        }
        if (ind1 == ind2)
        {
            end = start;
        }
        else
        {
            end = goToRow(*sentinelNode, ind2, (*textLength));
        }
        //Save sublist for reattatchment in case of undo and redos
        newCommand->comType = 'd';
        newCommand->sublistHead = start;
        newCommand->sublistTail = end;
        newCommand->nodesBefore = ind2 - ind1 + 1;
        newCommand->nodesAfter = 0;
        //Cut sublist
        cutSublist(&start, &end);
        (*textLength) -= (ind2 - ind1 + 1);
    }
    else
    {
        newCommand->comType = 'n';
        newCommand->sublistHead = NULL;
        newCommand->sublistTail = NULL;
        newCommand->nodesBefore = 0;
        newCommand->nodesAfter = 0;
    }
    pushCommandStack(undoStackHead, newCommand);
}

void undoText(tTextNode **sentinelHead, tCommandStackNode *toUndo, int *textLength)
{
    switch (toUndo->comType)
    {
    case 'c':;
        //Swap saved sublist with the sublist created by the change command
        tTextNode *newSublistHead = toUndo->sublistHead->prev->next;
        tTextNode *newSublistTail = toUndo->sublistTail->next->prev;
        //Cut new sublist
        cutSublist(&newSublistHead, &newSublistTail);
        //Attach old sublist
        pasteSublist(&(toUndo->sublistHead), &(toUndo->sublistTail));
        //Save new sublist in the command stack
        (toUndo->sublistHead) = newSublistHead;
        (toUndo->sublistTail) = newSublistTail;
        //Update text length
        (*textLength) -= (toUndo->nodesAfter - toUndo->nodesBefore);
        break;
    case 'a':
        //Remove sublist
        cutSublist(&(toUndo->sublistHead), &(toUndo->sublistTail));
        //Update text length
        (*textLength) -= (toUndo->nodesAfter - toUndo->nodesBefore);
        break;
    case 'd':
        //Reattach old sublist
        pasteSublist(&(toUndo->sublistHead), &(toUndo->sublistTail));
        //Update text length
        (*textLength) += (toUndo->nodesBefore);
        break;
    }
}

void redoText(tTextNode **sentinelHead, tCommandStackNode *toRedo, int *textLength)
{
    switch (toRedo->comType)
    {
    case 'c':;
        //Swap saved sublist with the sublist created by the change command
        tTextNode *newSublistHead = toRedo->sublistHead->prev->next;
        tTextNode *newSublistTail = toRedo->sublistTail->next->prev;
        //Cut new sublist
        cutSublist(&newSublistHead, &newSublistTail);
        //Attach old sublist
        pasteSublist(&(toRedo->sublistHead), &(toRedo->sublistTail));
        //Save new sublist in the command stack
        (toRedo->sublistHead) = newSublistHead;
        (toRedo->sublistTail) = newSublistTail;
        (*textLength) += (toRedo->nodesAfter - toRedo->nodesBefore);
        break;
    case 'a':
        //Reattach sublist
        pasteSublist(&(toRedo->sublistHead), &(toRedo->sublistTail));
        //Update text length
        (*textLength) += (toRedo->nodesAfter - toRedo->nodesBefore);
        break;
    case 'd':
        //Re-remove sublist
        cutSublist(&(toRedo->sublistHead), &(toRedo->sublistTail));
        //Update text length
        (*textLength) -= (toRedo->nodesBefore);
        break;
    }
}

void undoRedo(tTextNode **sentinelNode, tCommandStackNode **toUndo, tCommandStackNode **toRedo, int *times, int *undoDone, int *textLength)
{
    if (*times >= 0)
    {
        //Undo
        for (int i = 1; i <= *times; i++)
        {
            undoText(sentinelNode, *toUndo, textLength);
            pushCommandStack(toRedo, popCommandStack(toUndo));
        }
    }
    else
    {
        //Redo
        for (int i = 1; i <= -*times; i++)
        {
            redoText(sentinelNode, *toRedo, textLength);
            pushCommandStack(toUndo, popCommandStack(toRedo));
        }
    }
    *undoDone=(*undoDone)+(*times);
    *times = 0;
}

int main(char argc, char *argv[])
{
    tCommandStackNode *toUndo = NULL;
    tCommandStackNode *toRedo = NULL;
    //Always points to the first command to be undone (prev is to be redone)
    tTextNode *sentinelNode = NULL;
    int textLength = 0;
    char command[MAXADDRESSLENGTH * 2 + 2] ={};
    char *param1 = NULL;
    char *param2 = NULL;
    char *from = NULL;
    int length = 0;
    int commandLength = 0;
    int undoDone = 0;
    char *commapos = NULL;
    int times = 0;
    //Add list sentinel
    sentinelNode = malloc(sizeof(tTextNode));
    sentinelNode->content = NULL;
    sentinelNode->next = sentinelNode;
    sentinelNode->prev = sentinelNode;
    //Read command
    fgets(command, sizeof(command), stdin);
    *(strchr(command, '\n')) = '\0';
    while (strcmp(command, "q") != 0)
    {
        switch (command[strlen(command) - 1])
        {
        case 'p':
        {
            //Do the required undos and redos
            undoRedo(&sentinelNode, &toUndo, &toRedo, &times, &undoDone, &textLength);
            //Format: X..X,Y..Yp
            param1 = malloc(MAXADDRESSLENGTH);
            param2 = malloc(MAXADDRESSLENGTH);
            //Get comma position to get first and last row to edit
            commapos = strchr(command, ',');
            //First row
            from = command;
            length = commapos - from;
            strncpy(param1, from, length);
            param1[length] = '\0';
            //Last row
            from = commapos + sizeof(char);
            length = strchr(commapos, 'p') - from;
            strncpy(param2, from, length);
            param2[length] = '\0';
            printText(sentinelNode, atoi(param1), atoi(param2), textLength);
            free(param1);
            free(param2);
            break;
        }
        case 'c':
        {
            //Do the required undos and redos
            undoRedo(&sentinelNode, &toUndo, &toRedo, &times, &undoDone, &textLength);
            for(;toRedo!=NULL;){
                free(popCommandStack(&toRedo));
            }
            commandLength-=undoDone;
            undoDone=0;
            //Format: X..X,Y..Yc
            param1 = malloc(MAXADDRESSLENGTH);
            param2 = malloc(MAXADDRESSLENGTH);
            //Get comma position to get first and last row to edit
            commapos = strchr(command, ',');
            //First row
            from = command;
            length = commapos - from;
            strncpy(param1, from, length);
            param1[length] = '\0';
            //Last row
            from = commapos + sizeof(char);
            length = strchr(commapos, 'c') - from;
            strncpy(param2, from, length);
            param2[length] = '\0';
            changeText(&sentinelNode, &toUndo, atoi(param1), atoi(param2), &textLength);
            commandLength++;
            free(param1);
            free(param2);
            break;
        }
        case 'd':
        {
            //Do the required undos and redos
            undoRedo(&sentinelNode, &toUndo, &toRedo, &times, &undoDone, &textLength);
            for(;toRedo!=NULL;){
                free(popCommandStack(&toRedo));
            }
            commandLength-=undoDone;
            undoDone=0;
            //Format: X..X,Y..Yd
            param1 = malloc(MAXADDRESSLENGTH);
            param2 = malloc(MAXADDRESSLENGTH);
            //Get comma position to get first and last row to edit
            commapos = strchr(command, ',');
            //First row
            from = command;
            length = commapos - from;
            strncpy(param1, from, length);
            param1[length] = '\0';
            //Last row
            from = commapos + sizeof(char);
            length = strchr(commapos, 'd') - from;
            strncpy(param2, from, length);
            param2[length] = '\0';
            deleteText(&sentinelNode, &toUndo, atoi(param1), atoi(param2), &textLength);
            commandLength++;
            free(param1);
            free(param2);
            break;
        }
        case 'u':
        {
            param1 = malloc(MAXADDRESSLENGTH);
            strncpy(param1, command, strlen(command) - 1);
            times += atoi(param1);
            if (times > commandLength - undoDone)
            {
                times = commandLength - undoDone;
            }
            free(param1);
            break;
        }
        case 'r':
        {
            param1 = malloc(MAXADDRESSLENGTH);
            strncpy(param1, command, strlen(command) - 1);
            times -= atoi(param1);
            if (times < -undoDone)
            {
                times = -undoDone;
            }
            free(param1);
            break;
        }
        }
        fgets(command, sizeof(command), stdin);
        *(strchr(command, '\n')) = '\0';
    }
    return 0;
}