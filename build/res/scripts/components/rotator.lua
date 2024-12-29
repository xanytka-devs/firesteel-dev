-- Runs on first frame
function onstart() 
	print("Rotator is here!")
end

-- Runs every frame
function onupdate()
	print("rotato")
end

-- Runs when entity is destroyed
function ondestroy()
	print("Ouch")
end

-- Runs on last frame (when application is closed)
function onend()
	print("See you next time")
end