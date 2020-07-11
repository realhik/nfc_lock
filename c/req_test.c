#include <zmq.h>
#include <string.h> // memcpy
#include <stdlib.h> //realloc
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#define ZMQ_NUM_IOTHREADS 1

// Reads the message body to a string, returns pointer you must free
char* msg_to_str(zmq_msg_t* msg)
{
    size_t size = zmq_msg_size(msg);
    if (size < 1)
    {
        return NULL;
    }
    char *string = malloc(size + 1);
    memcpy(string, zmq_msg_data(msg), size);
    string[size] = 0x0; // Force last byte to null
    return (string);
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage:\n  req_test tcp://localhost:7070 card_uid\n");
        return 1;
    }
    int err;
    // TODO: IFDEF on the API version ? we use 2.2 due to raspbian not having 3.2 packages
    void *context = zmq_init(ZMQ_NUM_IOTHREADS);
    void *requester = zmq_socket(context, ZMQ_REQ);
    err = zmq_connect(requester, argv[1]);
    if (err != 0)
    {
        printf("ERROR: zmq_connect failed with %s\n", zmq_strerror(zmq_errno()));
        goto END;
    }

    zmq_msg_t request;
    int uidlen = strlen(argv[2]);
    err = zmq_msg_init_size(&request, uidlen);
    if (err != 0)
    {
        printf("ERROR: zmq_msg_init_size failed with %s\n", zmq_strerror(zmq_errno()));
        goto END;
    }
    memcpy(zmq_msg_data(&request), argv[2], uidlen);
    printf("Sending request\n");
    err = zmq_sendmsg(requester, &request, 0);
    if (err != 0)
    {
        printf("ERROR: zmq_send failed with %s\n", zmq_strerror(zmq_errno()));
        goto END;
    }
    zmq_msg_close(&request);

    printf("Waiting for response\n");
    int partno=0;
    while (1)
    {
        partno++;
        zmq_msg_t message;
        zmq_msg_init(&message);
        if (err != 0)
        {
            printf("ERROR: zmq_msg_init failed with %s\n", zmq_strerror(zmq_errno()));
            goto END;
        }
        err = zmq_recvmsg(requester, &message, 0);
        if (err != 0)
        {
            zmq_msg_close (&message);
            printf("ERROR: zmq_recv failed with %s\n", zmq_strerror(zmq_errno()));
            goto END;
        }

        printf("Received part %d, %d bytes\n", partno, (int)zmq_msg_size(&message));

        // Read the body as string
        char* body = msg_to_str(&message);
        printf("==\n%s\n==\n", body);
        free(body);

        // Done with message
        zmq_msg_close (&message);
        
        // See if we have more parts
        int64_t more;
        size_t more_size = sizeof(more);
        err = zmq_getsockopt(requester, ZMQ_RCVMORE, &more, &more_size);
        if (err != 0)
        {
            printf("ERROR: zmq_getsockopt failed with %s\n", zmq_strerror(zmq_errno()));
            goto END;
        }
        if (!more)
        {
            break;
        }
    }
    printf("All %d parts received\n", partno);

END:
    zmq_close(requester);
    zmq_term(context);
    return err;
}
