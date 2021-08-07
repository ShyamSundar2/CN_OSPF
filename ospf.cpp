#include <bits/stdc++.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
#include <functional>
#include <filesystem>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include<time.h>
#include <mutex> 
using namespace std;

#define MAX_Routers 40
#define MAX_time 41
#define MAXLINE 255
#define PORT1 10000         // PORT NUMBER.

int Interval[3];


int SeqHello = 0;           // Number of times Hello Pakects are exchanged .
int SeqLSA = 0;             // Number of times LSA Pakects are exchanged .
int SeqSPF = 0;             // Number of times Routing Table is generated.

string infile;              // Input File name
string outfile;             // Output File name
ofstream Ofile;             // File stream of Outfile
ifstream ifile;             // File stream of Infile

int Sid;                    // Node ID.
int N;                      // Total Number of Nodes.

int sockfd;                 // SocketID for exchange of hello Packets
int sockfd1;                // SocketID for exchange of LSA Packets

int check_count = 0;        // Buffer Variable Used for synchronisation.

/* Function Used for Printing In Table Format */
template<typename T> void Print(T t, const int& width) 
{
    Ofile << left<<setw(width)<< t;
}

/* Function To check Input */
void inputcheck()
{
    cout<<Sid<<" "<<infile<<" "<<outfile;
    for(int i = 0;i < 3;i++)
    {
        cout<<" "<<Interval[i];
    }
    cout<<endl;
}



class Graph
{
private:
    int V;                                          // Total Number of Nodes.
    int Sourceid;                                   // Operating Node ID.

    int graph[MAX_Routers][MAX_Routers];            // This Matrix is changed according to exchange in Hello packets.
    int LSAgraph[MAX_Routers][MAX_Routers];         // This Matrix is changed according to exchange in LSA packets.
    int SPFgraph[MAX_Routers][MAX_Routers];         // This Matrix is to store the previous LSAgraph.

    vector<pair< int,pair< int,int > >> adjl;       // Left edges for the Node.
    vector<pair< int,pair< int,int > >> adjr;       // Right edges for the Node.

    bool S[MAX_Routers];                            // Set Used for dijkstra's algorithm.    
    int PRED[MAX_Routers];                          // Stores the parent of the Node in the we get after dijkstra's.
    int D[MAX_Routers];                             // Cost Used for dijkstra's algorithm.

public:
    /*  Constructor  */
    Graph(int a,int b)
    {
        // cout<<"YESSSSSSSSSSSSSSSSSs"<<endl;
        V = a;
        Sourceid = b;
        for(int i = 0;i<V;i++)
        {
            for(int j = 0;j<V;j++)
            {
                graph[i][j] = 0;
                LSAgraph[i][j] = 0;
                SPFgraph[i][j] = 0;
            }
        }
        for(int i = 0;i<V;i++)
        {
            S[i] = false;
            PRED[i] = -1;
            D[i] = INT_MAX;
        }
    }

    /*  adds an edge in graph and adjl,adjr  */
    void add_edge(int x,int y,int mic,int mac)
    {
        graph[x][y] = 1;
        graph[y][x] = 1;
        if(x == Sourceid)
            adjl.push_back(make_pair(y,make_pair(mic,mac)));
        if(y == Sourceid)
            adjr.push_back(make_pair(x,make_pair(mic,mac)));
    }


    /* Stores all the left edges in given input vector */
    void left(vector<int> &a)
    {
        for(int i = 0;i<adjl.size();i++)
            a.push_back(adjl[i].first);
    }

     /* Stores all the right edges in given input vector */
    void right(vector<int> &a)
    {
        for(int i = 0;i<adjr.size();i++)
            a.push_back(adjr[i].first);
    }

    /* Generates the random number between the Given range and also updates the graph */
    int generate_edge(int j)
    {
        srand(time(0));
        for(int i = 0;i<adjr.size();i++)
        {
            if(adjr[i].first == j)
            {
                pair<int,int> xx = adjr[i].second;
                int x = xx.first + (rand()%(xx.second - xx.first + 1));
                graph[Sourceid][j] = x;
                graph[j][Sourceid] = x;
                return x;
            }
        }

        return -1;
    }


    /* Update an edge in graph */
    void update_edge(int i,int j,int c)
    {
        graph[i][j] = c;
        graph[j][i] = c;
    }

    /* Update an edge in Lsagraph according to the LSA packet */
    bool UPD_LSA_edge(string s,int i,int se)
    {
        istringstream ss(s);
        string LSA;
        int seqn,srcid,n;

        ss>> LSA >> srcid >> seqn >> n;
        
        if(srcid != i || se != seqn)
            return false;

        for(int i=0;i<n;i++)
        {
            int j,c;
            ss >> j >> c;
            LSAgraph[j][srcid] = c;
            LSAgraph[srcid][j] = c;
        }
        return true;
    }


    /* Generates the LSA Message */
    string GEN_LSA_MSG()
    {
        string s = "LSA " + to_string(Sourceid) + " " + to_string(SeqLSA) + " " 
                        + to_string(adjl.size());

        for(int i = 0;i<adjl.size();i++)
        {
            s += " " + to_string(adjl[i].first) + " " + to_string(graph[Sourceid][adjl[i].first]);
            LSAgraph[adjl[i].first][Sourceid] = graph[Sourceid][adjl[i].first];
            LSAgraph[Sourceid][adjl[i].first] = graph[Sourceid][adjl[i].first];
        }
        return s;
    }


    /* Dijkstras Algorithm */
    void dijkstra()
    {
        // Initialization.
        int count = 1;
        S[Sourceid] = true;
        D[Sourceid] = 0;
        for(int i = 0;i<V;i++)
        {
            if(graph[Sourceid][i])
            {
                D[i] = SPFgraph[Sourceid][i];
                PRED[i] = Sourceid;
            }
        }
        
        while(count < V) // LOOP
        {
            int min = -1;
            int w;
            for(int i = 0;i<V;i++)
            {
                if(!S[i] && (min == -1 || min > D[i]))
                {
                    min = D[i];
                    w = i;
                }
            }
            S[w] = true;
            // cout<<w<<endl;
            count++;
            for(int i = 0; i < V;i++ )
            {
                if(!S[i] && graph[w][i])
                {
                    if(D[w] != INT_MAX && D[i] > D[w] + SPFgraph[w][i])
                    {
                        D[i] = D[w] + SPFgraph[w][i];
                        PRED[i] = w;
                    } 
                }
            }
            // for(int i = 0;i<V;i++)
            //     cout<<PRED[i]<<" ";
            // cout<<endl;
        }
    }


    /* Prints the Routing Table in the Outfile*/
    void printpaths()
    {
        Ofile.open(outfile, ios::out | ios::app );
        string s = "Routing Table for Node No.  "+ to_string(Sourceid)+" at Time "+to_string((SeqSPF+1)*Interval[2]);
        Print(s,38);
        Ofile<<endl;
        Print("Destination",15);
        Print("Path",17);
        Print("Cost",6);
        Ofile<<endl;
        for(int i = 0;i<V;i++)
        {
            Print(i,15);
            int id = i;
            stack<int> s;
            s.push(id);
            while(PRED[id] != -1)
            {
                s.push(PRED[id]);
                id = PRED[id];
            }
            string S;
            S.push_back('0' + s.top());
            s.pop();
            while(!s.empty())
            {
                S.push_back('-');
                S.push_back('0' + s.top());
                s.pop();
            }
            Print(S,17);
            Print(D[i],6);
            Ofile<<endl;
        }
        Ofile<<endl;
        Ofile<<endl;
        Ofile.close();
    }


    /* To Update SPF graph after every LSA update */
    void updateSPF()
    {
        for(int i = 0;i<V;i++)
        {
            for(int j = 0;j<V;j++)
            {
                SPFgraph[i][j] = LSAgraph[i][j];
            }
        }
    }

    /* Buffer Function To check LSA graph */
    void printLSAgraph()
    {
        Ofile.open(outfile, ios::out | ios::app );
        cout<<"PRINTING_LSA\n";
        for(int i = 0;i<V;i++)
        {
            for(int j = 0;j<V;j++)
            {
                Ofile<<LSAgraph[i][j]<<" ";
            }
            Ofile<<endl;
        }
        Ofile<<endl;
        Ofile<<endl;
        Ofile.close();
    }

    /* Buffer Function To check HELLO graph */
    void printgraph()
    {
        Ofile.open(outfile, ios::out | ios::app );
        for(int i = 0;i<V;i++)
        {
            for(int j = 0;j<V;j++)
            {
                Ofile<<graph[i][j]<<" ";
            }
            Ofile<<endl;
        }
        Ofile<<endl;
        Ofile.close();
    }

    /* intializes the Arrays used in Dijkstra's*/
    void initialize()
    {
        for(int i = 0;i<V;i++)
        {
            S[i] = false;
            PRED[i] = -1;
            D[i] = INT_MAX;
        }
    }
};



/* This function Helps in excuting a given function periodically given interval*/
void timer_start(std::function<void(Graph*)> func,Graph* arg,const unsigned int interval)
{
    std::thread([func,arg, interval]() {
        while (true)
        {
            if(SeqHello >= MAX_time)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
            func(arg);
        }
    }).detach();
}


/* Creates a Socket according to flag */
void create_socket()
{
      struct sockaddr_in servaddr;
      
    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
      
    memset(&servaddr, 0, sizeof(servaddr));
      
    servaddr.sin_family    = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT1 + Sid);

    if ( bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 )
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

}


/*Used To Send the given message To a router*/
void SEND(string hello,int i)
{
    struct sockaddr_in	 servaddr;
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT1 + i);
    servaddr.sin_addr.s_addr = INADDR_ANY;
    
    int n;
    socklen_t len;

    int x;
    x = sendto(sockfd, hello.c_str(), strlen(hello.c_str()),MSG_CONFIRM, (const struct sockaddr *) &servaddr,sizeof(servaddr));
        
    // printf("%d Hello message sent.\n",x);
}

queue<string> HelloBuffer;
queue<string> LSABuffer;

string RECEIVE(bool flag)
{
    if(flag && !HelloBuffer.empty())
    {
        string s = HelloBuffer.front();
        HelloBuffer.pop();
        return s;
    }
    if(flag && HelloBuffer.empty())
    {
        char buffer[MAXLINE];
        struct sockaddr_in cliaddr;
        int n;
        socklen_t len;
        string str;
        do
        {
            bzero(buffer,MAXLINE);
            n = recvfrom(sockfd, (char *)buffer, MAXLINE,MSG_WAITALL, (struct sockaddr *) &cliaddr,&len);
            buffer[n] = '\0';
            str.assign(buffer,buffer+n);
            if(strncmp(buffer,"HELLO",5) != 0)
            	LSABuffer.push(str);
        } while (strncmp(buffer,"HELLO",5) != 0);
        return str;
    }

    if(!LSABuffer.empty())
    {
        string s = LSABuffer.front();
        LSABuffer.pop();
        return s;
    }
    char buffer[MAXLINE];
    struct sockaddr_in cliaddr;
    int n;
    socklen_t len;
    string str;
    do
    {
        bzero(buffer,MAXLINE);
        n = recvfrom(sockfd, (char *)buffer, MAXLINE,MSG_WAITALL, (struct sockaddr *) &cliaddr,&len);
        buffer[n] = '\0';
        str.assign(buffer,buffer+n);
        if(strncmp(buffer,"HELLO",5) == 0)
        	HelloBuffer.push(str);
    } while (strncmp(buffer,"HELLO",5) == 0);
    return str;
}


/* LSA Packets are exchanged HERE*/
void SendLsa(Graph *gg)
{
    vector<int>a;
    gg->left(a);
    gg->right(a);
    string s = gg->GEN_LSA_MSG();
    for(int i = 0;i<N;i++)                                      // Sequentially Generating the LSA packets and sending Them.
    {
        if(Sid == i)
        { 
           for(int j = 0;j<a.size();j++)
                SEND(s,a[j]);
        }
        else
        {
            char buffer[MAXLINE];
            struct sockaddr_in cliaddr;
            int n;
            socklen_t len;
            string str;
            do
            {
                str = RECEIVE(false);
            }
            while(!gg->UPD_LSA_edge(str,i,SeqLSA));         // Continues Untill We got the required Message.
            for(int j = 0;j<a.size();j++)
                SEND(str,a[j]);
        }
    }
        gg->updateSPF();
    //   gg->printLSAgraph();
    SeqLSA++;
}

/*Hello Packets are exchanged here*/
void SendHELLO(Graph *gg)
{
    
     vector<int>ll;
    gg->left(ll);

    vector<int>rr;
    gg->right(rr);

    for(int i = 0;i<ll.size();i++)                  // Sending all the HELLO packets for that Node.
    {
        string hello = "HELLO ";
        hello += to_string(Sid);
        SEND(hello,ll[i]);
    }

    for(int i = 0;i<rr.size()+ll.size();i++)                 // Receiving The Hello and HELLO REPLY packets 
    {
        string s = RECEIVE(true);
        istringstream ss(s); 
        string hh;
        ss>>hh;
        if(hh == "HELLO")
        {
            int xx;
            ss >> xx;
            int yy = gg->generate_edge(xx);
            string Hlrply = "HELLOREPLY " + to_string(Sid) + " "+to_string(xx)+" "+to_string(yy);
            SEND(Hlrply,xx);
        }
        else
        {
            int X[3];
            ss >> X[0] >> X[1] >> X[2];
            gg->update_edge(X[0],X[1],X[2]);
        }
    }

    cout<<"HELLO DONE\n";

    SeqHello++;

    // gg->printgraph();
}

/* Calls the function For printing the Output*/
void PRINT_SPF(Graph *gg)
{
    gg->dijkstra();
    gg->printpaths();
    gg->initialize();
    SeqSPF++;
}

int main(int argc, char *argv[])
{
    if (argc < 6)
       {printf("ERORR\n");}

    /* Taking Inputs*/

    for(int i = 0;i < 3;i++)
    {
        stringstream ss = stringstream(argv[i+4]);
        ss >> Interval[i];
    }
    stringstream ss = stringstream(argv[1]);
    ss >> Sid;
    infile = argv[2];
    outfile = argv[3];

    Ofile.open(outfile);
    Ofile.close();

    ifile.open(infile);
    ifile >> N;
     Graph gg(N,Sid);
    int E;
    ifile >> E;
    while(E--)
    {
        int x,y,i,j;
        ifile>>x>>y>>i>>j;
        gg.add_edge(x,y,i,j);
    }
    ifile.close();

    /* Taking Inputs*/


    create_socket();

    timer_start(SendHELLO  ,&gg  ,Interval[0]*1000);        // Thread calls HELLO Packets Generating Function.
    timer_start(SendLsa    ,&gg  ,Interval[1]*1000);        // Thread calls LSA Packets Generating Function.
    timer_start(PRINT_SPF  ,&gg  ,Interval[2]*1000);        // Thread Which Prints Outputs after Given SPFInterval.

     while(1)
     {
        if(SeqHello >= MAX_time)            // Breaks When Max time is Reached.
            break;
     }
    close(sockfd);
}
