Emodule-Productivity
====================

This emodule helps you to fight against working distractions (procrastination), it consist in a similar technique to pomodoro with breaktimes but a lot more powerful, because it gives or deny's you access to the distracting windows. 

 * Define the windows that you consider important in the _Work tools_ section, for example: Terminal, Meld, Gvim, Calculator, Inkscape, xpad, etc...
 * Set the minutes that you want to have for _breaktime_ after every _x_ minutes of work
 * Start working and enjoy :)


Features
========

When you are working, it hides all the windows that you have not defined as _tools for work_, when the breaktime is reached it shows you them back allowing you some minutes of access to distracting things like chats or web, then finally hides them again by controlling for you the time that you lose on them (notifying you with a sound 5 seconds before to hide them)

There's an option for allow you to satisfy the urgent windows if they appears, only one time


Development
===========

The next development (TODO) of this emodule is for now frozen because it does his job, it will be maintained for future fixes and updates if needed, but we are not going to include more features for now on the near future, not because we don't have interest but because we don't have time.

If you found it interesting and you want to *collaborate by making it better*, you are more than welcome to do it and we are interested in adding more features.

After to study a bit the psycology behind procrastination, we leave here one of the most useful articles found: http://blog.iqmatrix.com/overcome-procrastination


TODO and Ideas
==============

 * *Profiles*: There's different types of tasks in our lifes, for example _boring_ and _creative_, the creative ones requires the less distractions possible and calm music, the boring tasks requires active music and fast breaktimes. By having different profiles we can have different types of works, selectable on the main window, that makes our desktop act differently. Also remember that different types of _jobs_ requires different configurations (more exactly different applications to use).

 * *Statistics*: One of the best ways to overcome procrastination is by *knowing* on what exactly _we lose time with_, we have already functions in the code to know how much times we have passed in every window, so we can make easly a way to show them to the user in *percentage bars* or a pie chart, we can also include the number of *keystrokes* used in every chart.

 * *Grant usage*: Sometimes we need a fast usage of a not-allowed application (like web), because we need for example a fast research in google, but at the same time is a focus of distraction, will be nice if by a _keyboard shortcut_ we call the module for grant us access to it, so it will ask us how much minutes we need for it and he will hide back the window after these minutes _(we are rational and sincere about the amount of time that we need, but we easly lose the control of the time passed, which is the task of this emodule)_, this feature will not allow to use again the application after passed a specific amount of time _'you already used it!'_

 * *Prohibited web access*: To have a list of comma-separated websites or keywords to pass to iptables for block us from access to the selected websites, this can give us the opportunity to use the web for our work but to not lose time in social websites (in fact we can easly lose time with any kind of website, specially new ones).

 * *Full Internet disconnection*: Internet is a focus of distractions, the optimal solution is of course to entire block it, by other side is possible that we still require some access to internet (apt-get, cronjobs of backups, etc), could be nice if is triggered some iptables rules that *blocks everything* except what we have listed to be allowed, just like the list of applications to use, but for internet keywords.

 * *Password-protected*: We are our own victims of our distractions, so is very easy for us to unload the module, click on stop-working, etc... these things should be not allowed/possible, maybe using a special password set by someone else

 * *Focus watcher*: That was one of the original ideas, to control _'which is the focused window'_ (the one you are working on), by giving features like re-focus on the correct window after an amount of time or to punish you if you lose too much time in the non-working window, this feature was replaced by the simply control of hiding windows, but some good ideas can be still recycled from this concept.

 * *Reclaim*: If the user has not typed or moved the mouse by a specific amount of time, reclaim his attention.

 * *Progress bar*: A progress-bar similar to the places emodule which turns from green to red per every cycle (breaktimes), this raises the reponsability of the user to focus on the work.

 * *Assessment and Monitoring*: In the same way that a personal trainer follows your progress, in every breaktime an entry box can ask you _'what you have reached on this cycle?'_, this data is very useful for a personal monitoring of progress since you can be conscious about your lacks and worth steps, this data can be also useful for show in a _daily statistics of progress and lacks_, other useful things can be asked like having a progress bar to set the value of efficiency, how much worth is, etc. _(see the article about overcome procrastination in the previous section)_.

 * *Goals*: This feature may change a bit the original behaviour of the emodule but is important to remember that we need to have a control/assesment of the time required for reach the goals, for example if we need 5 hours for finish something, we need to force us to use only 5 hours, instant rewards are a good incentive (to watch some funny video series), and punishing us in some way if we need more time than the decided one (blocking with iptables youporn for one week :))

 * *Postponement*: In the case that we have an assessment feature that knows which task are we doing at every moment, if we reach the double of time originally requested for finish a task, block the desktop for some minutes asking the user to reconsider whether it is worth what is trying to reach.



Tips
====

A free account on mindomo or using a slate is a very good way to _note_ everything that comes to your head and that you cannot do right now, this helps you freeing the pointers in your mind by moving away the ideas but not losing track of them

If you want to *try this application* but you can't compile it, an alternative and fast way is to directly try it from the live mode of Elive http://www.elivecd.org (from a bigger version than _2.0 - Topaz_)





