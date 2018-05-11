Trigger.prototype.StartCutscene = function(data)
{
	let cmpCinemaManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_CinemaManager);
	cmpCinemaManager.AddCinemaPathToQueue("test");
	cmpCinemaManager.Play();
};

{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.DoAfterDelay(1 * 1000, "StartCutscene", {});
}
