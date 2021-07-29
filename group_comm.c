#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <dirent.h>

#define port 4321

pthread_mutex_t lock;

struct sockaddr_in localSock;
struct ip_mreq globalgroup;

char username[20];
char groups[100][100];
char groupip[100][100];
int group_index = 0;
bool group_creator = false;
bool message_sender = false;

char addedgroups[100][100];
char addedgroupsip[100][100];
int addedgroupsindex = 0;

bool send_message_to_group(char*,int,int);
void send_message_to_everyone (int,char *);

bool polling = false;
bool poll_initiator = false;
bool enablepoll = false;
time_t time_to_poll =20;
time_t start_time;
int yes = 0, no=0;

int sd,rd;

char filenames[100][100];
int filenum;
bool file_searcher = false;
bool file_getter = false;


int create_socket()
{
   int fd = socket(AF_INET, SOCK_DGRAM, 0);
   if(fd < 0)
{
  perror("Opening datagram socket error");
  exit(1);
}

  return fd;
	
}


void reuse_port(int fd)
{
int reuse = 1;
if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0)
{
perror("Setting SO_REUSEADDR error");
close(fd);
exit(1);
}

}


void bind_socket(int fd)
{
memset((char *) &localSock, 0, sizeof(localSock));
localSock.sin_family = AF_INET;
localSock.sin_port = htons(port);
localSock.sin_addr.s_addr = INADDR_ANY;

if(bind(fd, (struct sockaddr*)&localSock, sizeof(localSock)))

{
perror("Binding datagram socket error");
close(fd);
exit(1);
}

}


void set_local_interface(int fd)
{
struct in_addr localInterface;
localInterface.s_addr = INADDR_ANY;
if(setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface)) < 0)
{
  perror("Setting local interface error");
  exit(1);
}

}


void add_global_group(int fd)
{
globalgroup.imr_multiaddr.s_addr = inet_addr("226.1.1.1");
globalgroup.imr_interface.s_addr = INADDR_ANY;

if(setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&globalgroup, sizeof(globalgroup)) < 0)

{
perror("Adding multicast group error");
close(fd);
exit(1);
}

char globalgrp[10] = "globalgrp";
strcpy(addedgroups[addedgroupsindex],globalgrp);
strcpy(addedgroupsip[addedgroupsindex],"226.1.1.1");
++addedgroupsindex;
}


void enable_loopback(int fd)
{
char loopch = 1;

if(setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopch, sizeof(loopch)) < 0)

{
perror("Setting IP_MULTICAST_LOOP error");
close(fd);
exit(1);
}

}

int store_file_names(void)
{
    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    int len = 0;
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            len++;
        }
        closedir(d);
    }
    d = opendir(".");

    int i=0;
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            sprintf(filenames[i++],"%s",dir->d_name);
          //  printf("%s \n",filenames[i-1]);
        }
    }

    return len;
}

char* decode_grp_name(char*databuf)
{
int ind =6; 
char *grp = (char*)malloc(1000*sizeof(char));
while(databuf[ind]!='\0')
{
	if(databuf[ind]=='@')
	break;
	grp[ind-6] = databuf[ind];
	ind++;
			
}
//printf("%d",ind);
grp[ind-6] ='\0';
//printf("new group created: %s\n",grp);
return grp;
}

char* decode_message(char*databuf)
{
char *message = (char*)malloc(10000*sizeof(char));
int ind =6; 
while(databuf[ind]!='@')
{
	ind++;			
}
int i =0;
ind++;
while(databuf[ind]!='#')
{
message[i++] = databuf[ind++];
}
message[i] ='\0';
return message;
}


char* decode_username(char*databuf)
{
char *user = (char*)malloc(1000*sizeof(char));
int ind =6; 
while(databuf[ind]!='#')
{
	ind++;			
}
int i =0;
while(databuf[ind]!='\0')
{
user[i++] = databuf[++ind];
}
user[i] ='\0';
return user;
}

int get_grp_index(char*groupname)
{
for(int i =0;i<group_index;i++)
{
	if(strcmp(groupname,groups[i])==0)
	{
	return i;
	}
}
return -1;
}

bool add_to_group(char *grpip)
{
struct ip_mreq newgroup;
newgroup.imr_multiaddr.s_addr = inet_addr(grpip);
newgroup.imr_interface.s_addr = INADDR_ANY;
if(setsockopt(rd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&newgroup, sizeof(newgroup)) < 0)
{
return false;
}
else
return true;
}

int get_added_group_index(char*groupname)
{
for(int i =0;i<addedgroupsindex;i++)
{
	if(strcmp(groupname,addedgroups[i])==0)
	{
	return i;
	}
}
return -1;
}

void response_to_poll(char*user,char*groupname)
{
char c[2];
if(!poll_initiator)
{
printf("To poll press 0\n");
while(!enablepoll && difftime(time(0),start_time)<time_to_poll);
}
else
enablepoll = true;
if(enablepoll)
{
printf("Enter your response(y/n): \n");
scanf("%s",c);
enablepoll = false;
if(difftime(time(0),start_time)>time_to_poll)
{
printf("poll not recorded");
return;
}
char message[100] = "$type4";
strcat(message,groupname);
strcat(message,"@");
strcat(message,c);
strcat(message,"#");
strcat(message,user);

int grp_index = get_added_group_index(groupname);

bool issend= send_message_to_group(message,grp_index,sizeof(message));
if(issend)
printf("poll recorded successfully\n");
else
printf("poll not recorded"); 
}
polling = false;
}

void upload_file(char*user,char*filename)
{
int fd, sz;
char c[1000]; 

fd = open(filename, O_RDONLY);
if (fd < 0) { perror("open"); }
  
sz = read(fd, c, sizeof(c));

c[sz] = '\0';
char message[1100] = "$type6";
strcat(message,"file@");
strcat(message,c);
strcat(message,"#");
strcat(message,username);

send_message_to_everyone(sd,message);
}

void send_all_files(char*groupname,char*user)
{
char message[1000] = "$type8";
strcat(message,groupname);
strcat(message,"@");
for(int i=0;i<filenum;i++)
{
strcat(message,filenames[i]);
strcat(message,"\n");
}
strcat(message,"#");
strcat(message,username);

int grp_index = get_added_group_index(groupname);
bool issend= send_message_to_group(message,grp_index,sizeof(message));
}


void send_message_to_everyone (int fd,char *message)
{

struct sockaddr_in groupSock;
int datalen;
char databuf[1024];


strcpy(databuf,message);
datalen = sizeof(databuf);

memset((char *) &groupSock, 0, sizeof(groupSock));

groupSock.sin_family = AF_INET;
groupSock.sin_addr.s_addr = inet_addr("226.1.1.1");
groupSock.sin_port = htons(port);

if(sendto(fd, databuf, datalen, 0, (struct sockaddr*)&groupSock, sizeof(groupSock)) < 0)
{
   perror("Sending datagram message error");
}

}


void *receive_message(void* fd)
{
int *rd = (int*)fd;

int datalen;
char databuf[1100];
sleep(1);
while(1){


datalen = sizeof(databuf); 
//pthread_mutex_lock(&lock);

if(read(*rd, databuf, datalen) < 0)
{
 perror("Reading datagram message error");
exit(1);
 continue;
}
else
{ pthread_mutex_lock(&lock);
//printf("Reading datagram message...OK.\n");
//printf("The message from multicast server is: \"%s\"\n", databuf);
char *grp;
char *message;
char *user;
int added;
if(databuf[0] == '$')
{
	switch(databuf[5])
	{
	case '1':
		
		grp = decode_grp_name(databuf);
	//	printf("%d\n",group_index);
		strcpy(groups[group_index],grp);
		strcpy(groupip[group_index],"226.2.1.");
		char ip[5];
		sprintf(ip, "%d", group_index+2);
		strcat(groupip[group_index],ip);
		if(group_creator)
		{
		printf("Group Created:%s\n",grp);
		printf("Group Created with ip:%s\n",groupip[group_index]);				
		group_creator = false;
		}
		group_index++;
		free(grp);
		pthread_mutex_unlock(&lock);
	//	sleep(6);
		
		break;
		
	case '2':if(!message_sender){
		grp = decode_grp_name(databuf);
	        added =get_added_group_index(grp);
		if(added!=-1)
		{
		message = decode_message(databuf);
		user = decode_username(databuf);
		printf("Message by %s from Group %s: %s\n",user,grp,message);
		free(grp);
		free(message);
		free(user);
		}		
		}
		message_sender = false;
		pthread_mutex_unlock(&lock);

		break;	
	case '3':
		grp = decode_grp_name(databuf);
		added =get_added_group_index(grp);
		if(added!=-1)
		{
		message = decode_message(databuf);
		user = decode_username(databuf);
		printf("Poll intitiated by %s from Group %s: %s\n",user,grp,message);
		polling = true;
		start_time = time(0);		
		response_to_poll(user,grp);
		free(grp);
		free(message);
		free(user);
		}
		pthread_mutex_unlock(&lock);	

		break;
	case '4':
		if(poll_initiator)
		{
		message = decode_message(databuf);
		if(strcmp(message,"y")==0 || strcmp(message,"Y")==0)
		++yes;
		else if(strcmp(message,"n")==0 || strcmp(message,"N")==0)
		++no;	
		free(message);
		}	
		pthread_mutex_unlock(&lock);

		break;	
		
	case '5':if(!file_searcher)
	       {
	
	        grp = decode_grp_name(databuf);	      	        	        
		message = decode_message(databuf);
		user = decode_username(databuf);
		for(int i =0;i<filenum;i++)
		 {
		if(strcmp(filenames[i],message)==0)
		   {  
		printf("You have the file %s needs.\n",user);			
		printf("Uploading file...\n");	
		sleep(2);
		printf("Uploaded!\n");
		upload_file(user,filenames[i]);				
		   }
		 }
		 		
		free(message);
		free(user);
		
                }
		pthread_mutex_unlock(&lock);
		
		break;
	case '6':if(file_searcher)
		{
		printf("Receiving file....\n");
		message = decode_message(databuf);
		int fd = open("received.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
		int sz = write(fd, message, strlen(message));
		//printf("%s\n",message);
		file_searcher = false;
		free(message);
		}		
		pthread_mutex_unlock(&lock);
		break;	
	case '7':if(!file_getter)
		{
		grp = decode_grp_name(databuf);
		user = decode_username(databuf);
		send_all_files(grp,user);
		free(grp);
	//	free(message);
		free(user);
		}
		
		pthread_mutex_unlock(&lock);
		break;	
	case '8':if(file_getter)
		{
		user = decode_username(databuf);
		message = decode_message(databuf);
		sleep(2);
		printf("Files of %s:\n",user);
		printf("%s\n",message);
		file_getter = false;
//		free(grp);
		free(message);
		free(user);
		}

		pthread_mutex_unlock(&lock);	
	default :
	        break;	
	}
}
else if(strcmp(databuf,"endpol")==0)
	{
		pthread_mutex_unlock(&lock);
		sleep(15);
	}

}
   // pthread_mutex_unlock(&lock);

}

}
bool send_message_to_group(char*message,int grp_index,int size)
{

struct sockaddr_in groupSock;
int datalen;

datalen = size;
memset((char *) &groupSock, 0, sizeof(groupSock));

groupSock.sin_family = AF_INET;
groupSock.sin_addr.s_addr = inet_addr(addedgroupsip[grp_index]);
groupSock.sin_port = htons(port);

if(sendto(sd, message, datalen, 0, (struct sockaddr*)&groupSock, sizeof(groupSock)) < 0)
{
   return false;
}

else
  return true;
}

void create_group()
{
char groupname[100];
char message[100] = "$type1";

printf("Enter the group name:");
scanf("%[^\n]s",groupname);
getchar();

char newip[20];
strcpy(newip,"226.2.1.");
char ip[5];
sprintf(ip, "%d", group_index+2);
strcat(newip,ip);

bool ifadded = add_to_group(newip);
if(ifadded)
{
strcpy(addedgroups[addedgroupsindex],groupname);
strcpy(addedgroupsip[addedgroupsindex],newip);
addedgroupsindex++;
group_creator = true;
strcat(message,groupname);
send_message_to_everyone(sd,message);
}
else
printf("Not able to create the group..\n");

}

void search_group()
{
char groupname[100];
printf("Enter the group name:");
scanf("%[^\n]s",groupname);
getchar();

int grp_index = get_grp_index(groupname);
if(grp_index==-1)
printf("No such group is Present\n");
else{
printf("Group is Present\n");
printf("ip:%s\n",groupip[grp_index]);
}
}

void join_group()
{
char groupname[100];
printf("Enter the group name:");
scanf("%[^\n]s",groupname);
getchar();

int grp_index = get_grp_index(groupname);

if(grp_index==-1)
{
 printf("no such group is present\n");
 return;
}

bool ifadded = add_to_group(groupip[grp_index]);
if(ifadded)
{
printf("Added to the group successfully..\n");
strcpy(addedgroups[addedgroupsindex],groupname);
strcpy(addedgroupsip[addedgroupsindex],groupip[grp_index]);
addedgroupsindex++;
}
else
printf("Not able to add to the group..\n");

}

void send_message()
{
message_sender = true;
char groupname[100];
char ampersand[2] ="@";
char usersign[2] = "#";

printf("Enter the group name:");
scanf("%[^\n]s",groupname);
getchar();
int grp_index = get_added_group_index(groupname);
if(grp_index==-1)
{
 printf("no such group is present\n");
 return;
}

char message[100] = "$type2";
strcat(message,groupname);
strcat(message,ampersand);

char sender_message[100];
printf("Enter the message:");
scanf("%[^\n]s",sender_message);
getchar();

strcat(message,sender_message);
strcat(message,usersign);
strcat(message,username);
//printf("%s\n",message);

bool issend= send_message_to_group(message,grp_index,sizeof(message));
if(issend)
printf("Message sent successfully\n");
else
printf("cannot send the message");


}

void createpoll()
{
char groupname[100];

printf("Enter the group name:");
scanf("%[^\n]s",groupname);
getchar();

int grp_index = get_added_group_index(groupname);
if(grp_index==-1)
{
 printf("no such group is present\n");
 return;
}
char message[100] = "$type3";
strcat(message,groupname);
strcat(message,"@");

char pollques[100];
printf("Enter the question:");
scanf("%[^\n]s",pollques);
getchar();

strcat(message,pollques);
strcat(message,"#");
strcat(message,username);

bool issend= send_message_to_group(message,grp_index,sizeof(message));
if(issend){
printf("poll sent successfully\n");
poll_initiator = true;
}
else
printf("cannot send the poll");
}

void display_poll()
{
printf("POLL RESULTS: \n");
char message[10] = "endpol";
send_message_to_everyone(sd,message);
if(poll_initiator){
printf("Total votes received: %d\n",yes+no);
if((yes+no)!=0){
printf("Number of People in Favour:%d (%d %%)\n",yes,100*yes/(yes+no));
printf("Number of People not in Favour:%d (%d %%)\n",no,100*no/(no+yes));
yes = 0;
no = 0;
}
}
else
{
yes = 0;
no = 0;
}
}

void search_file()
{
file_searcher = true;
char filename[100];
printf("Enter the filename:");
scanf("%[^\n]s",filename);
getchar();
for(int i =1;i<addedgroupsindex;i++)
{
char message[100] = "$type5";
strcat(message,addedgroups[addedgroupsindex]);
strcat(message,"@");
strcat(message,filename);
strcat(message,"#");
strcat(message,username);
bool issend= send_message_to_group(message,i,sizeof(message));
}


}

void get_file_names()
{
file_getter = true;
char groupname[100];
printf("Enter the group name:");
scanf("%[^\n]s",groupname);
getchar();

int grp_index = get_added_group_index(groupname);
if(grp_index==-1)
{
 printf("no such group is present\n");
 return;
}
char message[100] = "$type7";
strcat(message,groupname);
strcat(message,"@#");
strcat(message,username);
bool issend= send_message_to_group(message,grp_index,sizeof(message));

}
void printMenu()
{   
    printf("\e[1;1H\e[2J");
    printf("\n#############################################################\n");
    printf("Press 1 to Create a Group \n");
    printf("Press 2 to Search for a Group \n");
    printf("Press 3 to Join a Group \n");
    printf("Press 4 to Send Message to a Group\n" );
    printf("Press 5 to Create a Poll \n");
    printf("Press 6 to Search a file in Group  \n");
    printf("Press 7 to Request file names from members of a Group\n");
   // printf("Press 8 to exit\n");
    printf("###############################################################\n \n");
}


int main(int argc,char **argv)
{
pthread_t thread_id;
int choice;


sd =create_socket();
set_local_interface(sd);
enable_loopback(sd);

rd =create_socket();
reuse_port(rd);
bind_socket(rd);
set_local_interface(rd);
add_global_group(rd);
filenum = store_file_names();

if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }


printf("\e[1;1H\e[2J");
printf("Enter your user name:");
scanf("%[^\n]",username);
getchar();
printf("Welcome %s!\n",username);
sleep(3);

pthread_create(&thread_id, NULL,receive_message,(void*) &rd);
pthread_mutex_unlock(&lock);
while(1)
{
if(!polling)
{
printMenu();
scanf("%d",&choice);
getchar();
if(choice==0)
{enablepoll = true;
continue;
}
printf("\e[1;1H\e[2J");
pthread_mutex_lock(&lock);


switch(choice)
{

case 1: create_group();
        pthread_mutex_unlock(&lock);
        sleep(4);
   	break;
case 2:	search_group();
	pthread_mutex_unlock(&lock);
	sleep(4);
	break;
case 3: join_group();
	pthread_mutex_unlock(&lock);
	sleep(4);
	break;
case 4: send_message(); 
	pthread_mutex_unlock(&lock);
	sleep(4);
	break;
case 5: createpoll();
	pthread_mutex_unlock(&lock);
	sleep(time_to_poll+2);
	display_poll();
	sleep(10);
	polling = false;
	break;
case 6: search_file();
	pthread_mutex_unlock(&lock);
	sleep(4);
	break;
case 7: get_file_names();
	pthread_mutex_unlock(&lock);
	break;
default:
	break;	
}


}
}
pthread_join(thread_id, NULL);

return 0;

}

