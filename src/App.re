/* Externals allow to interact with existing JavaScript code or modules. In this
   case, we want to "import" an image:
   https://melange.re/v4.0.0/communicate-with-javascript.html#using-functions-from-other-javascript-modules
   The code below will generate `import CamelFunJpg from "./img/camel-fun.jpg";`
   so that Esbuild can process the asset later on.
   */
[@mel.module "./img/camel-fun.jpg"] external camelFun: string = "default";

/*
 * In OCaml, a module is similar to a TypeScript or JavaScript module.
 * It's a way to group related functions, types, and components. Here, `App`
 * is a module that encapsulates our main app component.
 *
 * The `[@react.component]` attribute is a special annotation in OCaml for
 * React. It's used to define a React component in a way that's more idiomatic
 * to OCaml. This attribute automatically manages the props transformation,
 * providing a seamless integration with React's component model.
 *
 * The `make` function inside the module is equivalent to defining a functional
 * component in React using JavaScript or TypeScript. In ReasonReact, it's
 * common to name the main function of a component `make` instead of the
 * component's name.
 */

// type loadingStatus =
//   | Loading
//   | Loaded(result(Feed.feed, string));

module App = {
  module P = Js.Promise;

  type step =
    | Idle /* There is no request going on, and no data to be shown. In this case, we will just show instructions to proceed with the request. */
    | Loading /* A request is currently taking place to fetch the feed. */
    | InvalidUsername /* The entered username is not a valid GitHub ID. */
    | Loaded(result(Feed.feed, string)); /* A request has finished, its result is contained inside the variant value itself. */

  type state = {
    username: string,
    step,
  };

  [@react.component]
  let make = () => {
    // let (data, setData) = React.useState(() => Loading);
    // let (username, setUsername) = React.useState(() => "jchavarri");
    let (state, setState) =
      React.useState(() => {username: "jchavarri", step: Idle});

    let fetchFeed = username => {
      setState(state => {...state, step: Loading});

      Fetch.fetch(
        "https://gh-feed.vercel.app/api?page=1&user="
        ++ Username.toString(username),
      )
      |> P.then_(response => {
           let status = Fetch.Response.status(response);
           switch (status) {
           | 200 =>
             Fetch.Response.text(response)
             |> P.then_(text => {
                  let data =
                    try(Ok(text |> Json.parseOrRaise |> Feed.feed_of_json)) {
                    | Json.Decode.DecodeError(msg) =>
                      Js.Console.error(msg);
                      Error("Failed to decode: " ++ msg);
                    };
                  setState(state => {...state, step: Loaded(data)});
                  P.resolve();
                })
             |> ignore
           | status =>
             /* Handle non-200 status */
             setState(state =>
               {
                 ...state,
                 step:
                   Loaded(
                     Error(
                       "Error: Received status " ++ string_of_int(status),
                     ),
                   ),
               }
             )
           };
           P.resolve();
         })
      |> ignore;
    };

    React.useEffect0(() => {
      switch (Username.make(state.username)) {
      | Ok(username) => fetchFeed(username)
      | Error () =>
        Js.Exn.raiseError(
          "Initial username passed to React.useState is invalid",
        )
      };
      None;
    });

    <>
      <div>
        <label htmlFor="username-input"> {React.string("Username:")} </label>
        <input
          id="username-input"
          value={state.username}
          onChange={event => {
            setState(_ =>
              {username: event->React.Event.Form.target##value, step: Idle}
            )
          }}
          onKeyDown={event => {
            let enterKey = 13;
            if (React.Event.Keyboard.keyCode(event) == enterKey) {
              switch (Username.make(state.username)) {
              | Ok(username) => fetchFeed(username)
              | Error () =>
                setState(state => {...state, step: InvalidUsername})
              };
            };
          }}
          placeholder="Enter GitHub username"
        />
      </div>
      {switch (state.step) {
       | InvalidUsername => <div> {React.string("Invalid username")} </div>
       | Idle =>
         <div>
           {React.string(
              "Press the \"Enter\" key to confirm the username selection.",
            )}
         </div>
       | Loading => <div> {React.string("Loading...")} </div>
       | Loaded(Error(msg)) => <div> {React.string(msg)} </div>
       | Loaded(Ok(feed)) =>
         <div>
           <h1> {React.string("GitHub Feed")} </h1>
           <ul>
             {switch (feed.entries) {
              | [||] => React.string("This user feed is empty")
              | entries =>
                entries
                |> Array.map((entry: Feed.entry) =>
                     <li key={entry.id}>
                       <h2> {React.string(entry.title)} </h2>
                       {switch (entry.content) {
                        | None => React.null
                        | Some(content) =>
                          <div dangerouslySetInnerHTML={"__html": content} />
                        }}
                     </li>
                   )
                |> React.array
              }}
           </ul>
         </div>
       }}
    </>;
  };
};

ReactDOM.querySelector("#root")
->(
    fun
    | Some(root_elem) => {
        let root = ReactDOM.Client.createRoot(root_elem);
        ReactDOM.Client.render(root, <App />);
      }
    | None =>
      Js.Console.error(
        "Failed to start React: couldn't find the #root element",
      )
  );
