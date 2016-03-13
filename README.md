# Tracking protection for Uzbl

This project adds tracking protection to Uzbl, based on [a Firefox feature](https://support.mozilla.org/en-US/kb/tracking-protection-pbm).
They've set up lists of stuff to block, and it [significantly speeds up browsing](http://ieee-security.org/TC/SPW2015/W2SP/papers/W2SP_2015_submission_32.pdf).
Moreover, they've got some great ways to rationalize the morality of this list.
I won't bore you with those reasons in this readme though.

## Setting it up

Run `make` to download the lists, preprocess them, and build the request checker.
Then, set up Uzbl by putting the following in your config (usually `~/.config/uzbl/config`):

```
set request_handler = sync_spawn ???/check-tracking
```

You'll have to fill in where you have the compiled check-tracking program.
There's a commented out line in the stock config that you can use as a template.
`sync_spawn` is available in the version of Uzbl that comes with Debian.
They have a new version (0.9) on their GitHub, where it's been renamed to `spawn_sync`.
Use that instead if you've got that version.
`spawn_sync` is deprecated though, so if you're using a future version of Uzbl where it's been removed, then you figure it out.

## Catalog of everything in this repo

**tables.h** declares some data structures to hold the blacklist and entity whitelist data.

**stove.py** converts JSON blacklist and entity whitelist files into **cooked.c**, which implements the declarations in *tables.h*.

**check-tracking.c** receives a request URI and page URI from Uzbl and accesses data structures declared in *tables.h* to check whether or not to block the request, by redirecting to `about:blank`.

## What Mozilla's tracking protection does

(If you know of official documentation on this topic, let me know if I have it wrong here.)
There are two lists, a *blacklist* and an *entity whitelist*.
Supposedly, these lists are prepared by a company called Disconnect.

The blacklist has a list of domain names.
If a request doesn't go to one of these domains, then it's allowed.
It doesn't have to be an exact match with the request URI's hostname: if `y.z` is on the list, a request to `http://w.x.y.z/` counts as a match.
If a request goes to a domain on the blacklist, then check the entity whitelist.
This list has special "Content" and "Legacy" sections which break some sites.
This project excludes those sections (as Firefox does in its default configuration), which compromises privacy for compatibility.

The entity whitelist connects sets of *properties* (user-facing websites) with *resources* (sites that serve files needed by the properties) for *entities* (companies).
For example, it contains information like Google's (an entity) `youtube.com` (a property) can access `doubleclick.net` (a resource).
If a request to a blacklisted domain is a resource to be loaded into a property both belonging to the same entity, the request is allowed.
Again, the request request and page URI hostnames just have to end in the domains on this list.
Otherwise, the request is blocked.
The effect is that each entity's knowledge of you is siloed--for example, Google can track you across its own sites, but not while you browse Facebook's sites.

## Remaining issues

* I don't have a good way to get "what" makes the request.
The check-tracking program currently uses the `UZBL_URI` environment variable, which is the URI of the page that's visible.
If you follow a link from `https://www.reddit.com/` to `https://www.twitter.com/`, it'll think that Reddit is trying to load a resource from Twitter, which it won't allow.
So navigation is pretty much broken.
I've currently made it so that you can always navigate to `about:blank`, and then navigate to anywhere.
Still sucks though.
