import { connect, ExampleAPI, DelayAddRequestT, EventDataFilterT, HelloWorldRequestT } from './lib/FlatBufferAPI'

const sleep = async (ms: number): Promise<void> => {
    return new Promise(resolve => {
        setTimeout(() => resolve(), ms);
    })
}

const main = async () => {

    const { result, message } = await connect({ address: '127.0.0.1', port: 8080 });
    console.log('connect result', result, message);

    let req = new HelloWorldRequestT();
    req.name = 'Song';

    let res = await ExampleAPI.helloWorld(req);
    console.log(res);

    let addReq = new DelayAddRequestT();
    addReq.a = 100;
    addReq.b = 200;

    let addRes = await ExampleAPI.delayAdd(addReq);
    console.log(addRes);

    ExampleAPI.subscribeObjectCreateEvent(new EventDataFilterT(42), event => console.log('receive event :', event.data));
}

main()