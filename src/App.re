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

type loadingStatus =
  | Loading
  | Loaded(result(Feed.feed, string));

module App = {
  [@react.component]
  let make = () => {
    let (data, setData) = React.useState(() => Loading);
    let (username, setUsername) = React.useState(() => "jchavarri");

    let fetchFeed = username => {
      Js.Promise.(
        Fetch.fetch("https://gh-feed.vercel.app/api?page=1&user=" ++ username)
        |> then_(Fetch.Response.text)
        |> then_(text =>
             {
               let data =
                 try(Ok(text |> Json.parseOrRaise |> Feed.feed_of_json)) {
                 | Json.Decode.DecodeError(msg) =>
                   Js.Console.error(msg);
                   Error("Failed to decode: " ++ msg);
                 };
               setData(_ => Loaded(data));
             }
             |> resolve
           )
      )
      |> ignore;
    };

    React.useEffect0(() => {
      fetchFeed(username);
      None;
    });

    <>
      <div>
        <label htmlFor="username-input"> {React.string("Username:")} </label>
        <input
          id="username-input"
          value=username
          onChange={event => {
            setUsername(event->React.Event.Form.target##value)
          }}
          onKeyDown={event => {
            let enterKey = 13;
            if (React.Event.Keyboard.keyCode(event) == enterKey) {
              setData(_ => Loading);
              fetchFeed(username);
            };
          }}
          placeholder="Enter GitHub username"
        />
      </div>
      {switch (data) {
       | Loading => <div> {React.string("Loading...")} </div>
       | Loaded(Error(msg)) => <div> {React.string(msg)} </div>
       | Loaded(Ok(feed)) =>
         <div>
           <h1> {React.string("GitHub Feed")} </h1>
           <ul>
             {feed.entries
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
              |> React.array}
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
